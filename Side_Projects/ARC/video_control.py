#!/usr/bin/env python3

from __future__ import annotations

import socket
import socketserver
import subprocess
import threading
from dataclasses import dataclass
from typing import Callable


CONTROL_HOST = "127.0.0.1"
CONTROL_PORT = 8765
MAX_CONTROL_BYTES = 255

MODE_IDLE = "idle"
MODE_RECORDING = "recording"
MODE_TRANSMITTING = "transmitting"
MODE_STREAMING = "streaming"


@dataclass(frozen=True)
class VideoCommands:
    recording: list[str]
    transmitting_camera: list[str]
    transmitting_encoder: list[str]
    transmitting_sink: list[str]
    streaming_camera: list[str]
    streaming_sink: list[str]


class VideoController:
    def __init__(
        self,
        commands: VideoCommands,
        launcher: Callable[..., subprocess.Popen] = subprocess.Popen,
        log: Callable[[str], None] | None = None,
        on_started: Callable[[], None] | None = None,
        on_stopped: Callable[[], None] | None = None,
    ) -> None:
        self.commands = commands
        self.launcher = launcher
        self.log = log or (lambda _msg: None)
        self.on_started = on_started or (lambda: None)
        self.on_stopped = on_stopped or (lambda: None)

        self.mode = MODE_IDLE
        self.last_error: str | None = None
        self.rpicam_process: subprocess.Popen | None = None
        self.av1_process: subprocess.Popen | None = None
        self.transmit_process: subprocess.Popen | None = None
        self.stream_process: subprocess.Popen | None = None
        self._lock = threading.RLock()

    def start_recording(self) -> str:
        with self._lock:
            return self._transition(MODE_RECORDING, self._start_recording_pipeline)

    def start_transmitting(self) -> str:
        with self._lock:
            return self._transition(MODE_TRANSMITTING, self._start_transmitting_pipeline)

    def start_streaming(self) -> str:
        with self._lock:
            return self._transition(MODE_STREAMING, self._start_streaming_pipeline)

    def stop(self) -> str:
        with self._lock:
            self._stop_pipeline(clear_error=True)
            return MODE_IDLE

    def status(self) -> str:
        with self._lock:
            self._refresh_state()
            if self.last_error:
                return self._format_error(self.last_error)
            return self.mode

    def handle_command(self, command: str) -> str:
        normalized = self._normalize_command(command)
        command_map = {
            "start_recording": self.start_recording,
            "start_transmitting": self.start_transmitting,
            "start_streaming": self.start_streaming,
            "stop": self.stop,
            "status": self.status,
        }
        callback = command_map.get(normalized)
        if callback is None:
            return self._format_error("unknown_command")
        return callback()

    def _transition(self, target_mode: str, starter: Callable[[], None]) -> str:
        self._refresh_state()
        if self.mode == target_mode and self.last_error is None:
            return self.mode
        if self.mode != MODE_IDLE:
            self._stop_pipeline(clear_error=True)
        try:
            starter()
        except Exception as exc:
            self._stop_pipeline(clear_error=False)
            reason = f"spawn_failed:{exc.__class__.__name__}"
            self.last_error = reason
            self.log(f"Video command failed: {reason}")
            return self._format_error(reason)
        self.mode = target_mode
        self.last_error = None
        self.on_started()
        return self.mode

    def _start_recording_pipeline(self) -> None:
        self.rpicam_process = self.launcher(self.commands.recording)

    def _start_transmitting_pipeline(self) -> None:
        self.rpicam_process = self.launcher(
            self.commands.transmitting_camera,
            stdout=subprocess.PIPE,
        )
        self.av1_process = self.launcher(
            self.commands.transmitting_encoder,
            stdin=self.rpicam_process.stdout,
            stdout=subprocess.PIPE,
        )
        self.transmit_process = self.launcher(
            self.commands.transmitting_sink,
            stdin=self.av1_process.stdout,
        )

    def _start_streaming_pipeline(self) -> None:
        self.rpicam_process = self.launcher(
            self.commands.streaming_camera,
            stdout=subprocess.PIPE,
        )
        self.stream_process = self.launcher(
            self.commands.streaming_sink,
            stdin=self.rpicam_process.stdout,
        )

    def _refresh_state(self) -> None:
        if self.mode == MODE_IDLE:
            return
        failed = []
        for name, proc in self._active_processes():
            if proc.poll() is not None:
                failed.append(name)
        if not failed:
            return
        reason = f"process_exited:{','.join(failed)}"
        self.log(f"Video pipeline exited unexpectedly: {reason}")
        self._stop_pipeline(clear_error=False)
        self.last_error = reason

    def _stop_pipeline(self, clear_error: bool) -> None:
        was_active = self.mode != MODE_IDLE
        for _name, proc in reversed(list(self._active_processes())):
            try:
                if proc.poll() is None:
                    proc.kill()
            except Exception:
                pass
            try:
                proc.wait(timeout=5)
            except Exception:
                pass

        self.rpicam_process = None
        self.av1_process = None
        self.transmit_process = None
        self.stream_process = None
        self.mode = MODE_IDLE
        if clear_error:
            self.last_error = None
        if was_active:
            self.on_stopped()

    def _active_processes(self) -> list[tuple[str, subprocess.Popen]]:
        processes = []
        for name, proc in (
            ("rpicam", self.rpicam_process),
            ("av1", self.av1_process),
            ("transmit", self.transmit_process),
            ("stream", self.stream_process),
        ):
            if proc is not None:
                processes.append((name, proc))
        return processes

    @staticmethod
    def _normalize_command(command: str) -> str:
        normalized = command.strip().lower().replace("-", "_")
        alias_map = {
            "start recording": "start_recording",
            "start transmitting": "start_transmitting",
            "start streaming": "start_streaming",
            "stop video": "stop",
        }
        return alias_map.get(normalized, normalized.replace(" ", "_"))

    @staticmethod
    def _format_error(reason: str) -> str:
        return f"error:{reason}"


class _ControlRequestHandler(socketserver.StreamRequestHandler):
    def handle(self) -> None:
        raw = self.rfile.readline(MAX_CONTROL_BYTES + 2)
        if not raw:
            return
        command = raw.decode("ascii", errors="ignore").strip()
        if not command:
            response = "error:bad_packet"
        else:
            response = self.server.controller.handle_command(command)
        self.wfile.write(response.encode("ascii", errors="ignore")[:MAX_CONTROL_BYTES] + b"\n")


class ThreadedVideoControlServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    allow_reuse_address = True
    daemon_threads = True

    def __init__(self, controller: VideoController, host: str = CONTROL_HOST, port: int = CONTROL_PORT) -> None:
        self.controller = controller
        super().__init__((host, port), _ControlRequestHandler)


def send_video_command(
    command: str,
    host: str = CONTROL_HOST,
    port: int = CONTROL_PORT,
    timeout: float = 1.0,
) -> str:
    try:
        with socket.create_connection((host, port), timeout=timeout) as sock:
            sock.sendall(command.encode("ascii", errors="ignore")[:MAX_CONTROL_BYTES] + b"\n")
            response = b""
            while not response.endswith(b"\n"):
                chunk = sock.recv(MAX_CONTROL_BYTES + 1)
                if not chunk:
                    break
                response += chunk
    except OSError:
        return "error:controller_unavailable"

    decoded = response.decode("ascii", errors="ignore").strip()
    if not decoded:
        return "error:controller_unavailable"
    return decoded

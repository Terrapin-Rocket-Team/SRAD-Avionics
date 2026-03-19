#!/usr/bin/env python3

from __future__ import annotations

from dataclasses import dataclass
import threading
import time
from typing import Callable


MAX_PAYLOAD_SIZE = 64

AB_TELEM_TYPE = 0
AVI_TELEM_TYPE = 1
AB_CMD_TYPE = 2
AVI_CMD_TYPE = 3
LIVE_CMD_TYPE = 4
NOSE_VIDEO_CMD_TYPE = 5
AB_VIDEO_CMD_TYPE = 6

HEARTBEAT_PREFIX = "hb"


@dataclass(frozen=True)
class FramedPacket:
    packet_type: int
    payload: bytes
    raw: bytes


class FramedPacketParser:
    def __init__(self) -> None:
        self._buffer = bytearray()

    def feed(self, data: bytes) -> list[FramedPacket]:
        if not data:
            return []
        self._buffer.extend(data)
        packets = []
        while len(self._buffer) >= 2:
            payload_length = self._buffer[1]
            frame_length = payload_length + 2
            if len(self._buffer) < frame_length:
                break
            raw = bytes(self._buffer[:frame_length])
            packet_type = raw[0]
            payload = raw[2:]
            del self._buffer[:frame_length]
            packets.append(FramedPacket(packet_type=packet_type, payload=payload, raw=raw))
        return packets


class TrafficMonitor:
    def __init__(
        self,
        stale_after_seconds: float = 5.0,
        now: Callable[[], float] = time.monotonic,
    ) -> None:
        self.stale_after_seconds = stale_after_seconds
        self.now = now
        self._lock = threading.Lock()
        self._last_rx_time: float | None = None
        self._packet_count = 0

    def record_packet(self) -> None:
        with self._lock:
            self._last_rx_time = self.now()
            self._packet_count += 1

    def compact_status(self) -> str:
        with self._lock:
            if self._last_rx_time is None:
                return "telem:none:0"
            age = self.now() - self._last_rx_time
            state = "ok" if age <= self.stale_after_seconds else "stale"
            return f"telem:{state}:{age:.1f}:{self._packet_count}"


def build_packet(packet_type: int, payload: bytes) -> bytes:
    if not 0 <= packet_type <= 0xFF:
        raise ValueError("packet type must fit in one byte")
    if len(payload) > MAX_PAYLOAD_SIZE:
        raise ValueError("payload too large")
    return bytes([packet_type, len(payload)]) + payload


def format_heartbeat_payload(sequence: int, telemetry_status: str, video_status: str) -> str:
    return f"{HEARTBEAT_PREFIX}:{sequence}:{telemetry_status}:{video_status}"


def normalize_ascii_command(command: str) -> str:
    return command.strip().lower().replace("-", "_").replace(" ", "_")


class VideoCommandRouter:
    def __init__(
        self,
        send_command: Callable[[str], str],
        local_packet_type: int,
    ) -> None:
        self.send_command = send_command
        self.local_packet_type = local_packet_type

    def route(self, packet: FramedPacket) -> tuple[str, bytes]:
        if packet.packet_type != self.local_packet_type:
            return ("forward", packet.raw)

        try:
            command = packet.payload.decode("ascii").strip()
        except UnicodeDecodeError:
            return ("respond", build_packet(self.local_packet_type, b"err:bad_packet"))

        if not command:
            return ("respond", build_packet(self.local_packet_type, b"err:bad_packet"))

        status = self.send_command(command)
        response = status.encode("ascii", errors="ignore")[:MAX_PAYLOAD_SIZE]
        return ("respond", build_packet(self.local_packet_type, response))

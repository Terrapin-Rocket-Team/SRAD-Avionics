"""BLE helpers for discovery, notification buffering, and command input."""

from __future__ import annotations

import asyncio
import contextlib
import queue
import threading

from bleak import BleakClient, BleakScanner

from .config import DROP_TO_REALTIME


def make_notification_handler(raw_q: asyncio.Queue[bytes]):
    def _on_notify(_sender, data: bytearray) -> None:
        try:
            raw_q.put_nowait(bytes(data))
        except asyncio.QueueFull:
            if not DROP_TO_REALTIME:
                return
            with contextlib.suppress(asyncio.QueueEmpty):
                raw_q.get_nowait()
            with contextlib.suppress(asyncio.QueueFull):
                raw_q.put_nowait(bytes(data))

    return _on_notify


async def find_device(name: str):
    devices = await BleakScanner.discover(timeout=6.0)
    for device in devices:
        if device.name == name:
            return device
    return None


def _start_console_reader(prompt: str) -> tuple[queue.Queue[object], threading.Event]:
    inbox: queue.Queue[object] = queue.Queue()
    stop_event = threading.Event()

    def _reader() -> None:
        while not stop_event.is_set():
            try:
                line = input(prompt)
            except BaseException as exc:
                inbox.put(exc)
                break
            else:
                inbox.put(line)

    thread = threading.Thread(target=_reader, name="airbrake-console", daemon=True)
    thread.start()
    return inbox, stop_event


async def command_console(client: BleakClient, running: dict[str, bool], write_uuid: str) -> None:
    print("[TX] Console ready. Type a line and press Enter to send to the FC.")
    print("[TX] Type 'exit' or 'quit' to close.")
    inbox, stop_event = _start_console_reader("FC> ")

    try:
        while running["open"]:
            try:
                item = inbox.get_nowait()
            except queue.Empty:
                await asyncio.sleep(0.05)
                continue
            except asyncio.CancelledError:
                break

            if isinstance(item, (EOFError, KeyboardInterrupt)):
                running["open"] = False
                break
            if isinstance(item, BaseException):
                raise item

            line = str(item).strip()
            if not line:
                continue
            if line.lower() in {"exit", "quit"}:
                running["open"] = False
                break

            payload = (line + "\n").encode("utf-8")
            try:
                await client.write_gatt_char(write_uuid, payload, response=False)
                print(f"[TX] {line}")
            except Exception as exc:  # pragma: no cover - hardware path
                print(f"[TX] write failed: {exc}")
    finally:
        stop_event.set()

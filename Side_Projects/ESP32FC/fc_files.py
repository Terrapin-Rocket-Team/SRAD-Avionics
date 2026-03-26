#!/usr/bin/env python3
"""Simple USB serial file utility for the flight computer.

Protocol:
  ls -> send "FILE/LS\\n" and print the returned listing
  cp -> send "FILE/CP <name>\\n", wait for FILE/BOF ... FILE/EOF, save bytes

Examples:
  python fc_files.py ls
  python fc_files.py cp flight.log
  python fc_files.py cp configs/main.txt .\\downloads\\main.txt
  python fc_files.py --port COM7
"""

from __future__ import annotations

import argparse
import shlex
import sys
import time
from pathlib import Path

import serial
from serial.tools import list_ports

DEFAULT_BAUD = 115200
DEFAULT_TIMEOUT = 0.2
DEFAULT_CONNECT_DELAY = 1.0
DEFAULT_RESPONSE_TIMEOUT = 5.0
DEFAULT_QUIET_TIME = 0.35
DEFAULT_COPY_TIMEOUT = 60.0
BOF_MARKER = b"FILE/BOF"
EOF_MARKER = b"FILE/EOF"


def default_outdir() -> Path:
    downloads = Path.home() / "Downloads"
    if downloads.exists():
        return downloads / "fc_files"
    return Path.cwd() / "fc_downloads"


def format_port(port: list_ports.ListPortInfo) -> str:
    parts = [port.device]
    if port.description and port.description != "n/a":
        parts.append(port.description)
    if port.manufacturer:
        parts.append(port.manufacturer)
    return " | ".join(parts)


def list_available_ports() -> list[list_ports.ListPortInfo]:
    return list(list_ports.comports())


def score_port(port: list_ports.ListPortInfo) -> int:
    haystack = " ".join(
        filter(
            None,
            [port.device, port.description, port.manufacturer, port.product, port.hwid],
        )
    ).lower()

    weights = {
        "stm": 8,
        "stlink": 8,
        "esp32": 8,
        "usb": 3,
        "cdc": 3,
        "serial": 2,
        "cp210": 6,
        "silicon labs": 5,
        "wch": 5,
        "ch340": 6,
        "ftdi": 5,
    }
    return sum(weight for key, weight in weights.items() if key in haystack)


def resolve_port(explicit_port: str | None) -> str:
    if explicit_port:
        return explicit_port

    ports = list_available_ports()
    if not ports:
        raise RuntimeError("No serial ports found. Connect the FC or pass --port explicitly.")

    if len(ports) == 1:
        return ports[0].device

    ranked = sorted(ports, key=score_port, reverse=True)
    if score_port(ranked[0]) > 0:
        top_score = score_port(ranked[0])
        ties = [port for port in ranked if score_port(port) == top_score]
        if len(ties) == 1:
            return ranked[0].device

    formatted = "\n".join(f"  {format_port(port)}" for port in ports)
    raise RuntimeError(
        "Multiple serial ports found. Pass --port.\n"
        f"Available ports:\n{formatted}"
    )


def safe_remote_path(name: str) -> Path:
    cleaned = name.strip().replace("\\", "/")
    parts = [part for part in cleaned.split("/") if part not in ("", ".")]
    if not parts:
        raise ValueError("Remote file name is empty.")
    if any(part == ".." for part in parts):
        raise ValueError("Remote file name cannot contain '..'.")
    return Path(*parts)


def open_serial(args: argparse.Namespace) -> serial.Serial:
    port = resolve_port(args.port)
    ser = serial.Serial(
        port=port,
        baudrate=args.baud,
        timeout=args.timeout,
        write_timeout=args.timeout,
    )
    time.sleep(args.connect_delay)
    ser.reset_input_buffer()
    return ser


def send_command(ser: serial.Serial, command: str) -> None:
    ser.reset_input_buffer()
    ser.write(command.encode("utf-8"))
    ser.flush()


def read_until_quiet(
    ser: serial.Serial,
    response_timeout: float,
    quiet_time: float,
) -> bytes:
    started = time.monotonic()
    last_rx: float | None = None
    chunks: list[bytes] = []

    while True:
        waiting = ser.in_waiting
        if waiting:
            chunk = ser.read(waiting)
            if chunk:
                chunks.append(chunk)
                last_rx = time.monotonic()
        else:
            now = time.monotonic()
            if last_rx is None:
                if now - started >= response_timeout:
                    raise TimeoutError("Timed out waiting for FILE/LS response.")
            elif now - last_rx >= quiet_time:
                return b"".join(chunks)
            time.sleep(0.02)


def receive_file(
    ser: serial.Serial,
    response_timeout: float,
    copy_timeout: float,
) -> bytes:
    started = time.monotonic()
    buffer = bytearray()
    content_start: int | None = None

    while True:
        waiting = ser.in_waiting
        if waiting:
            chunk = ser.read(waiting)
            if chunk:
                buffer.extend(chunk)

        if content_start is None:
            bof_index = buffer.find(BOF_MARKER)
            if bof_index != -1:
                content_start = bof_index + len(BOF_MARKER)
                if buffer[content_start : content_start + 2] == b"\r\n":
                    content_start += 2
                elif buffer[content_start : content_start + 1] in (b"\r", b"\n"):
                    content_start += 1

        if content_start is not None:
            eof_index = buffer.find(EOF_MARKER, content_start)
            if eof_index != -1:
                return bytes(buffer[content_start:eof_index])

        elapsed = time.monotonic() - started
        if content_start is None and elapsed >= response_timeout:
            raise TimeoutError("Timed out waiting for FILE/BOF.")
        if elapsed >= copy_timeout:
            raise TimeoutError("Timed out waiting for FILE/EOF.")
        time.sleep(0.02)


def do_ls(ser: serial.Serial, args: argparse.Namespace) -> int:
    send_command(ser, "FILE/LS\n")
    payload = read_until_quiet(
        ser,
        response_timeout=args.response_timeout,
        quiet_time=args.quiet_time,
    )
    text = payload.decode("utf-8", errors="replace").strip()
    if not text:
        print("(no files returned)")
        return 0
    print(text)
    return 0


def destination_for_copy(
    remote_name: str,
    destination: str | None,
    outdir: Path,
) -> Path:
    if destination:
        return Path(destination).expanduser().resolve()
    return (outdir / safe_remote_path(remote_name)).resolve()


def do_cp(
    ser: serial.Serial,
    args: argparse.Namespace,
    remote_name: str,
    destination: str | None = None,
) -> int:
    target = destination_for_copy(remote_name, destination, args.outdir)
    target.parent.mkdir(parents=True, exist_ok=True)

    send_command(ser, f"FILE/CP {remote_name}\n")
    payload = receive_file(
        ser,
        response_timeout=args.response_timeout,
        copy_timeout=args.copy_timeout,
    )
    target.write_bytes(payload)
    print(f"Saved {len(payload)} bytes to {target}")
    return 0


def run_shell(args: argparse.Namespace) -> int:
    with open_serial(args) as ser:
        print(f"Connected to {ser.port} @ {ser.baudrate}")
        print(f"Default save folder: {args.outdir}")
        print("Commands: ls, cp <remote-name> [destination], exit")

        while True:
            try:
                raw = input("fc> ").strip()
            except EOFError:
                print()
                return 0
            except KeyboardInterrupt:
                print()
                return 0

            if not raw:
                continue

            try:
                parts = shlex.split(raw)
            except ValueError as exc:
                print(f"Parse error: {exc}")
                continue

            cmd = parts[0].lower()
            if cmd in {"exit", "quit"}:
                return 0
            if cmd == "ls":
                try:
                    do_ls(ser, args)
                except Exception as exc:  # noqa: BLE001
                    print(f"ls failed: {exc}")
                continue
            if cmd == "cp":
                if len(parts) < 2:
                    print("Usage: cp <remote-name> [destination]")
                    continue
                remote_name = parts[1]
                destination = parts[2] if len(parts) >= 3 else None
                try:
                    do_cp(ser, args, remote_name, destination)
                except Exception as exc:  # noqa: BLE001
                    print(f"cp failed: {exc}")
                continue

            print("Unknown command. Use ls, cp <remote-name> [destination], exit")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="List and copy files from the FC over USB serial."
    )
    parser.add_argument("--port", help="Serial port, for example COM7.")
    parser.add_argument(
        "--baud",
        type=int,
        default=DEFAULT_BAUD,
        help=f"Serial baud rate. Default: {DEFAULT_BAUD}.",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=DEFAULT_TIMEOUT,
        help=f"Serial read/write timeout in seconds. Default: {DEFAULT_TIMEOUT}.",
    )
    parser.add_argument(
        "--connect-delay",
        type=float,
        default=DEFAULT_CONNECT_DELAY,
        help=f"Delay after opening the port. Default: {DEFAULT_CONNECT_DELAY}s.",
    )
    parser.add_argument(
        "--response-timeout",
        type=float,
        default=DEFAULT_RESPONSE_TIMEOUT,
        help=f"Seconds to wait for the FC to start responding. Default: {DEFAULT_RESPONSE_TIMEOUT}s.",
    )
    parser.add_argument(
        "--quiet-time",
        type=float,
        default=DEFAULT_QUIET_TIME,
        help=f"How long ls waits after the last byte before finishing. Default: {DEFAULT_QUIET_TIME}s.",
    )
    parser.add_argument(
        "--copy-timeout",
        type=float,
        default=DEFAULT_COPY_TIMEOUT,
        help=f"Max time to wait for FILE/EOF during cp. Default: {DEFAULT_COPY_TIMEOUT}s.",
    )
    parser.add_argument(
        "--outdir",
        type=Path,
        default=default_outdir(),
        help=f"Default destination folder for cp. Default: {default_outdir()}.",
    )
    parser.add_argument(
        "--list-ports",
        action="store_true",
        help="Print detected serial ports and exit.",
    )

    subparsers = parser.add_subparsers(dest="command")
    subparsers.add_parser("ls", help="Request and print the FC file list.")

    cp_parser = subparsers.add_parser("cp", help="Copy one file from the FC.")
    cp_parser.add_argument("name", help="Remote file name to request from the FC.")
    cp_parser.add_argument(
        "destination",
        nargs="?",
        help="Optional local output path. Defaults to --outdir/<remote-name>.",
    )

    subparsers.add_parser("shell", help="Start an interactive prompt.")
    return parser


def print_ports() -> int:
    ports = list_available_ports()
    if not ports:
        print("No serial ports found.")
        return 0
    for port in ports:
        print(format_port(port))
    return 0


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    args.outdir = args.outdir.expanduser()

    if args.list_ports:
        return print_ports()

    if args.command in (None, "shell"):
        return run_shell(args)

    try:
        with open_serial(args) as ser:
            if args.command == "ls":
                return do_ls(ser, args)
            if args.command == "cp":
                return do_cp(ser, args, args.name, args.destination)
    except KeyboardInterrupt:
        print()
        return 130
    except Exception as exc:  # noqa: BLE001
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    parser.error(f"Unsupported command: {args.command}")
    return 2


if __name__ == "__main__":
    raise SystemExit(main())

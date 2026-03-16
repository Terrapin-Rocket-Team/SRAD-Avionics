"""Application entrypoint for the live BLE telemetry viewer."""

from __future__ import annotations

import asyncio
import contextlib
import sys
from typing import Any

from bleak import BleakClient

from .ble import command_console, find_device, make_notification_handler
from .config import (
    BLE_CONNECT_TIMEOUT_SECS,
    DEVICE_NAME,
    NUS_NOTIFY_UUID,
    NUS_SERVICE_UUID,
    NUS_WRITE_UUID,
    RAW_Q_MAX,
)
from .plotting import Dashboard
from .protocol import AviTelemetryStreamParser
from .state import TelemetryHistory


async def rx_consumer(
    raw_q: asyncio.Queue[bytes],
    parser: AviTelemetryStreamParser,
    history: TelemetryHistory,
    running: dict[str, bool],
) -> None:
    while running["open"]:
        try:
            chunk = await raw_q.get()
        except asyncio.CancelledError:
            break

        for sample in parser.feed(chunk):
            history.append(sample)


def _client_attempts() -> list[tuple[str, dict[str, Any]]]:
    attempts: list[tuple[str, dict[str, Any]]] = [
        (
            "targeted service discovery",
            {
                "timeout": BLE_CONNECT_TIMEOUT_SECS,
                "services": [NUS_SERVICE_UUID],
            },
        ),
        (
            "full service discovery",
            {
                "timeout": BLE_CONNECT_TIMEOUT_SECS,
            },
        ),
    ]

    if sys.platform == "win32":
        return [
            (
                "targeted uncached service discovery",
                {
                    "timeout": BLE_CONNECT_TIMEOUT_SECS,
                    "services": [NUS_SERVICE_UUID],
                    "winrt": {"use_cached_services": False},
                },
            ),
            *attempts,
        ]

    return attempts


async def connect_ble(device) -> BleakClient:
    errors: list[str] = []

    for index, (label, client_kwargs) in enumerate(_client_attempts(), start=1):
        client = BleakClient(device, **client_kwargs)
        try:
            print(f"[INIT] BLE attempt {index}: {label}")
            await client.connect()
            if not client.is_connected:
                raise RuntimeError("client reported disconnected state after connect")
            return client
        except Exception as exc:
            errors.append(f"{label}: {type(exc).__name__}: {exc}")
            print(f"[WARN] BLE attempt {index} failed: {type(exc).__name__}: {exc}")
            with contextlib.suppress(Exception):
                if client.is_connected:
                    await client.disconnect()

    raise RuntimeError(" ; ".join(errors))


async def main() -> None:
    print(f"[INIT] Searching for {DEVICE_NAME}...")
    device = await find_device(DEVICE_NAME)
    if not device:
        print(f"[ERROR] Device '{DEVICE_NAME}' not found")
        return

    raw_q: asyncio.Queue[bytes] = asyncio.Queue(maxsize=RAW_Q_MAX)
    history = TelemetryHistory()
    parser = AviTelemetryStreamParser()
    running = {"open": True}
    dashboard: Dashboard | None = None

    print(f"[INIT] Found {device.address}, connecting...")
    client: BleakClient | None = None
    try:
        client = await connect_ble(device)
        if not client.is_connected:
            print("[ERROR] BLE connect failed")
            return

        service_count = sum(1 for _ in client.services)
        print(f"[INIT] Connected in BLE, resolved {service_count} service(s)")
        notify_char = client.services.get_characteristic(NUS_NOTIFY_UUID)
        write_char = client.services.get_characteristic(NUS_WRITE_UUID)
        if notify_char is None:
            print(f"[ERROR] Notify characteristic {NUS_NOTIFY_UUID} was not discovered")
            return
        if write_char is None:
            print(f"[ERROR] Write characteristic {NUS_WRITE_UUID} was not discovered")
            return

        await client.start_notify(NUS_NOTIFY_UUID, make_notification_handler(raw_q))
        print("[INIT] Notifications started")
        print("[INIT] Opening dashboard...")
        dashboard = Dashboard()
        dashboard.bind_close(running)

        rx_task = asyncio.create_task(rx_consumer(raw_q, parser, history, running))
        plot_task = asyncio.create_task(dashboard.run(history, running))
        cmd_task = asyncio.create_task(command_console(client, running, NUS_WRITE_UUID))

        try:
            while running["open"]:
                await asyncio.sleep(0.2)
        except (KeyboardInterrupt, asyncio.CancelledError):
            pass
        finally:
            running["open"] = False
            for task in (cmd_task, plot_task, rx_task):
                task.cancel()
                with contextlib.suppress(asyncio.CancelledError, Exception):
                    await task
            with contextlib.suppress(Exception):
                await client.stop_notify(NUS_NOTIFY_UUID)
            if dashboard is not None:
                dashboard.close()
    except Exception as exc:
        print(f"[ERROR] BLE startup failed: {type(exc).__name__}: {exc}")
    finally:
        with contextlib.suppress(Exception):
            if client is not None and client.is_connected:
                await client.disconnect()

    print("[EXIT] Done")


def run() -> None:
    asyncio.run(main())

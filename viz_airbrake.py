#!/usr/bin/env python3
"""
Airbrake-focused BLE telemetry viewer.

What this version does:
- Plots only pressure and flap angle over time.
- Uses a longer rolling window (default 5 minutes).
- Adds a live command console so you can send commands to the FC over BLE NUS.

Requires:
  pip install bleak matplotlib
"""

import asyncio
import contextlib
import math
import re
import time
from collections import deque

import matplotlib.pyplot as plt
from bleak import BleakClient, BleakScanner


# ----------------------------
# Config
# ----------------------------
DEVICE_NAME = "ESP32-NUS-3"
NUS_NOTIFY_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"  # peripheral notify -> host
NUS_WRITE_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"   # host write -> peripheral

LINE_PREFIXES = ("TTELEM/", "TELEM/", "CTLM/")

# Long enough for full sweep + margin.
WINDOW_SECS = 300
ASSUMED_RATE_HZ = 20
MAX_POINTS = WINDOW_SECS * ASSUMED_RATE_HZ

UI_MAX_HZ = 8.0
RAW_Q_MAX = 5000
DROP_TO_REALTIME = True


# ----------------------------
# Runtime storage
# ----------------------------
t_s = deque(maxlen=MAX_POINTS)
pressure = deque(maxlen=MAX_POINTS)
flap_desired = deque(maxlen=MAX_POINTS)
flap_actual = deque(maxlen=MAX_POINTS)

RAW_Q: asyncio.Queue[bytes] = asyncio.Queue(maxsize=RAW_Q_MAX)
_line_buf = bytearray()
_host_t0 = time.monotonic()
_last_sig = None


# ----------------------------
# Parsing
# ----------------------------
INDEX = {
    "time": 0,
    "airbrake_desired": 9,
    "airbrake_actual": 10,
    "pressure": 15,
}


def _try_float(raw: str, default=0.0) -> float:
    s = raw.strip()
    if not s:
        return default
    try:
        v = float(s)
        return default if math.isnan(v) else v
    except ValueError:
        return default


def _split_on_prefixes(line: str):
    pattern = "|".join(re.escape(p) for p in LINE_PREFIXES)
    return [p for p in re.split(f"(?={pattern})", line) if p.strip()]


def _extract_payload(line: str):
    s = line.strip()
    for pref in LINE_PREFIXES:
        if s.startswith(pref):
            return s[len(pref):].strip()
    return None


def _parse_telem_line(line: str):
    payload = _extract_payload(line)
    if payload is None:
        return None

    parts = [p.strip() for p in payload.split(",")]
    if len(parts) <= INDEX["pressure"]:
        return None

    ts = _try_float(parts[INDEX["time"]], default=0.0)
    if ts <= 0.0:
        ts = time.monotonic() - _host_t0

    p = _try_float(parts[INDEX["pressure"]])
    fd = _try_float(parts[INDEX["airbrake_desired"]])
    fa = _try_float(parts[INDEX["airbrake_actual"]])
    return ts, p, fd, fa


# ----------------------------
# BLE
# ----------------------------
def _on_notify(_sender, data: bytes):
    try:
        RAW_Q.put_nowait(data)
    except asyncio.QueueFull:
        if not DROP_TO_REALTIME:
            return
        with contextlib.suppress(asyncio.QueueEmpty):
            RAW_Q.get_nowait()
        with contextlib.suppress(asyncio.QueueFull):
            RAW_Q.put_nowait(data)


async def _find_device(name: str):
    devices = await BleakScanner.discover(timeout=6.0)
    for d in devices:
        if d.name == name:
            return d
    return None


# ----------------------------
# Tasks
# ----------------------------
async def _rx_consumer(running):
    global _line_buf, _last_sig

    while running["open"]:
        try:
            chunk = await RAW_Q.get()
        except asyncio.CancelledError:
            break

        _line_buf.extend(chunk)
        if b"\n" not in _line_buf:
            continue

        *lines, remain = _line_buf.split(b"\n")
        _line_buf = bytearray(remain)

        for raw in lines:
            line = raw.decode("utf-8", errors="ignore").strip("\r")
            if not line:
                continue

            parsed_any = False
            for sub in _split_on_prefixes(line):
                parsed = _parse_telem_line(sub)
                if not parsed:
                    continue
                parsed_any = True

                ts, p, fd, fa = parsed
                sig = (round(ts, 3), round(p, 3), round(fd, 2), round(fa, 2))
                if sig == _last_sig:
                    continue
                _last_sig = sig

                t_s.append(ts)
                pressure.append(p)
                flap_desired.append(fd)
                flap_actual.append(fa)

            # Print non-telem lines so status/diagnostic messages are visible.
            if not parsed_any and not line.startswith(("TELEM/", "TTELEM/", "CTLM/")):
                print(f"[RX] {line}")


def _set_ylim(ax, values, default_span=10.0, pad_frac=0.1):
    if not values:
        return
    y_min = min(values)
    y_max = max(values)
    span = y_max - y_min
    if span < default_span:
        mid = 0.5 * (y_min + y_max)
        half = 0.5 * default_span
        ax.set_ylim(mid - half, mid + half)
        return
    pad = max(default_span * 0.1, span * pad_frac)
    ax.set_ylim(y_min - pad, y_max + pad)


async def _plot_updater(fig, ax_p, ax_f, ln_p, ln_fd, ln_fa, txt_p, txt_f, running):
    last_draw_t = 0.0
    last_len = 0

    while running["open"]:
        if not t_s:
            plt.pause(0.001)
            await asyncio.sleep(0.05)
            continue

        now = time.monotonic()
        cur_len = len(t_s)
        if cur_len == last_len or (now - last_draw_t) < (1.0 / UI_MAX_HZ):
            plt.pause(0.001)
            await asyncio.sleep(0.01)
            continue

        last_len = cur_len
        last_draw_t = now

        t0 = t_s[0]
        xs = [v - t0 for v in t_s]
        yp = list(pressure)
        yfd = list(flap_desired)
        yfa = list(flap_actual)

        ln_p.set_data(xs, yp)
        ln_fd.set_data(xs, yfd)
        ln_fa.set_data(xs, yfa)

        xmax = xs[-1] if xs[-1] > 5 else WINDOW_SECS
        xmin = max(0, xmax - WINDOW_SECS)
        ax_p.set_xlim(xmin, xmax)
        ax_f.set_xlim(xmin, xmax)

        _set_ylim(ax_p, yp, default_span=50.0)
        _set_ylim(ax_f, yfd + yfa, default_span=15.0)

        txt_p.set_text(f"P: {yp[-1]:.2f}")
        txt_f.set_text(f"D: {yfd[-1]:.1f} deg | A: {yfa[-1]:.1f} deg")

        fig.canvas.draw_idle()
        plt.pause(0.001)
        await asyncio.sleep(0.001)


async def _command_console(client: BleakClient, running):
    print("[TX] Console ready. Type a line and press Enter to send to FC.")
    print("[TX] Type 'exit' or 'quit' to close.")

    while running["open"]:
        try:
            line = await asyncio.to_thread(input, "FC> ")
        except (EOFError, KeyboardInterrupt):
            running["open"] = False
            break
        except asyncio.CancelledError:
            break

        line = line.strip()
        if not line:
            continue
        if line.lower() in {"exit", "quit"}:
            running["open"] = False
            break

        payload = (line + "\n").encode("utf-8")
        try:
            await client.write_gatt_char(NUS_WRITE_UUID, payload, response=False)
            print(f"[TX] {line}")
        except Exception as e:
            print(f"[TX] write failed: {e}")


# ----------------------------
# Main
# ----------------------------
async def main():
    print(f"[INIT] Searching for {DEVICE_NAME}...")
    dev = await _find_device(DEVICE_NAME)
    if not dev:
        print(f"[ERROR] Device '{DEVICE_NAME}' not found")
        return

    print(f"[INIT] Found {dev.address}, connecting...")
    async with BleakClient(dev) as client:
        if not client.is_connected:
            print("[ERROR] BLE connect failed")
            return

        await client.start_notify(NUS_NOTIFY_UUID, _on_notify)
        print("[INIT] Notifications started")

        plt.ion()
        fig, (ax_p, ax_f) = plt.subplots(2, 1, figsize=(12, 8), sharex=True)
        fig.suptitle("Pressure + Flap Angle (Live)")

        (ln_p,) = ax_p.plot([], [], linewidth=2.0, label="Pressure")
        (ln_fd,) = ax_f.plot([], [], linewidth=2.0, label="Flap Desired")
        (ln_fa,) = ax_f.plot([], [], linewidth=2.0, linestyle="--", label="Flap Actual")

        ax_p.set_ylabel("Pressure")
        ax_p.grid(True, alpha=0.3)
        ax_p.legend(loc="upper left", frameon=False)

        ax_f.set_ylabel("Flap Angle (deg)")
        ax_f.set_xlabel("Time (s, rolling)")
        ax_f.grid(True, alpha=0.3)
        ax_f.legend(loc="upper left", frameon=False)

        badge = dict(
            fontsize=9,
            bbox=dict(boxstyle="round,pad=0.25", facecolor="white", alpha=0.85, edgecolor="#ddd"),
        )
        txt_p = ax_p.text(0.98, 0.9, "", transform=ax_p.transAxes, ha="right", va="top", **badge)
        txt_f = ax_f.text(0.98, 0.9, "", transform=ax_f.transAxes, ha="right", va="top", **badge)

        plt.tight_layout(rect=(0.0, 0.0, 1.0, 0.95))
        plt.show(block=False)

        running = {"open": True}

        def _on_close(_evt):
            running["open"] = False

        fig.canvas.mpl_connect("close_event", _on_close)

        rx_task = asyncio.create_task(_rx_consumer(running))
        plot_task = asyncio.create_task(
            _plot_updater(fig, ax_p, ax_f, ln_p, ln_fd, ln_fa, txt_p, txt_f, running)
        )
        cmd_task = asyncio.create_task(_command_console(client, running))

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

    print("[EXIT] Done")


if __name__ == "__main__":
    asyncio.run(main())

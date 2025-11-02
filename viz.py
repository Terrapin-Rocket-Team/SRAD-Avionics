import asyncio, csv, time, io, sys
from collections import deque
from bleak import BleakScanner, BleakClient
import matplotlib.pyplot as plt
import matplotlib.animation as animation

DEVICE_NAME = "ESP32-NUS-3"  # your advertised name
NUS_NOTIFY_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"  # TX notify
LINE_PREFIX = "TELEM2/"

# Rolling buffers (~20s at 50 Hz; adjust as needed)
WINDOW_SECS = 60
ASSUMED_RATE_HZ = 2
MAX_POINTS = WINDOW_SECS * ASSUMED_RATE_HZ

t = deque(maxlen=MAX_POINTS)
alt_ft = deque(maxlen=MAX_POINTS)
lat = deque(maxlen=MAX_POINTS)
lon = deque(maxlen=MAX_POINTS)
acc = deque(maxlen=MAX_POINTS)

# CSV logger
log_path = f"telemetry_{int(time.time())}.csv"
log_file = open(log_path, "w", newline="")
csvw = csv.writer(log_file)
csvw.writerow(["t_s", "alt_ft_rel", "lat", "lon", "acc"])  # header

# Notification line-assembler
_line_buf = bytearray()

def _parse_line(line: str):
    # Expect: TELEM2/<t>,<alt>,<vel>,<acc>,<x>,<y>
    if not line.startswith(LINE_PREFIX):
        return None
    try:
        payload = line[len(LINE_PREFIX):]
        parts = payload.split(",")
        if len(parts) < 6:
            return None
        ts = float(parts[0])
        alt = float(parts[1])
        vel_val = float(parts[2])  # not used, but in data
        a = float(parts[3])
        latitude = float(parts[4])  # x
        longitude = float(parts[5])  # y
        return ts, alt, latitude, longitude, a
    except Exception:
        return None

def _on_notify(_, data: bytes):
    global _line_buf
    _line_buf.extend(data)
    # split by newline (ESP sends '\n'; handles '\r\n' too)
    *lines, remainder = _line_buf.split(b"\n")
    _line_buf = bytearray(remainder)
    for raw in lines:
        ln = raw.decode("utf-8", errors="ignore").strip("\r")
        parsed = _parse_line(ln)
        if parsed:
            ts, alt, latitude, longitude, a = parsed
            t.append(ts)
            alt_ft.append(alt)
            lat.append(latitude)
            lon.append(longitude)
            acc.append(a)
            csvw.writerow([ts, alt, latitude, longitude, a])

async def _find_device(name: str):
    devs = await BleakScanner.discover(timeout=6.0)
    for d in devs:
        if d.name == name:
            return d
    return None


# --- pretty 3-subplot UI with min Y-span enforcement ---
import asyncio, contextlib
import matplotlib
# If needed: matplotlib.use("TkAgg")  # or "QtAgg"
import matplotlib.pyplot as plt

MIN_Y_SPAN = 10.0  # <- never zoom in more than this

# Optional unit labels (edit to taste)
UNIT_ALT = "ft"
UNIT_ACC = "m/s²"

def _enforce_min_ylim(ax, min_span: float):
    y0, y1 = ax.get_ylim()
    span = y1 - y0
    if span < min_span:
        mid = (y0 + y1) / 2.0
        ax.set_ylim(mid - min_span/2.0, mid + min_span/2.0)

async def _plot_updater(fig, axes, lines, anns, window_secs, running_flag):
    ax1, ax2, ax3 = axes
    ln_alt, ln_path, ln_current, ln_acc = lines
    ann_alt, ann_latlon, ann_acc = anns

    while running_flag["open"]:
        if t:
            t0 = t[0]
            xs = [ti - t0 for ti in t]
            y_alt = list(alt_ft)
            y_lat = list(lat)
            y_lon = list(lon)
            y_acc = list(acc)

            # Altitude graph
            ln_alt.set_data(xs, y_alt)

            # GPS path on map
            ln_path.set_data(y_lon, y_lat)
            # Current position marker
            if y_lon and y_lat:
                ln_current.set_data([y_lon[-1]], [y_lat[-1]])

            # Acceleration graph
            ln_acc.set_data(xs, y_acc)

            # keep a 60s window (or your WINDOW_SECS) for time-based graphs
            xmax = xs[-1] if xs[-1] > 5 else window_secs
            xmin = max(0, xmax - window_secs)
            ax1.set_xlim(xmin, xmax)
            ax3.set_xlim(xmin, xmax)

            # autoscale altitude and acceleration with min span
            for ax in (ax1, ax3):
                ax.relim(); ax.autoscale_view(True, True, True)
                _enforce_min_ylim(ax, MIN_Y_SPAN)

            # autoscale the map view
            if y_lon and y_lat:
                ax2.relim()
                ax2.autoscale_view(True, True, True)
                # Add padding to map view
                lon_min, lon_max = min(y_lon), max(y_lon)
                lat_min, lat_max = min(y_lat), max(y_lat)
                lon_range = lon_max - lon_min
                lat_range = lat_max - lat_min
                # Ensure minimum range for visibility
                if lon_range < 0.0001:
                    lon_range = 0.0001
                if lat_range < 0.0001:
                    lat_range = 0.0001
                padding = 0.1
                ax2.set_xlim(lon_min - lon_range * padding, lon_max + lon_range * padding)
                ax2.set_ylim(lat_min - lat_range * padding, lat_max + lat_range * padding)

            # update "current value" badges
            ann_alt.set_text(f"{y_alt[-1]:.1f} {UNIT_ALT}".strip())
            ann_latlon.set_text(f"{y_lat[-1]:.6f}°, {y_lon[-1]:.6f}°".strip())
            ann_acc.set_text(f"{y_acc[-1]:.2f} {UNIT_ACC}".strip())

            fig.canvas.draw_idle()
            plt.pause(0.001)  # let GUI process events

        await asyncio.sleep(0.1)  # ~10 Hz UI refresh

async def main():
    dev = await _find_device(DEVICE_NAME)
    if not dev:
        print(f"Device '{DEVICE_NAME}' not found."); return

    async with BleakClient(dev) as client:
        if not client.is_connected:
            print("BLE connect failed."); return

        await client.start_notify(NUS_NOTIFY_UUID, _on_notify)

        # --- 3 stacked subplots: Alt, Map, Acc ---
        plt.ion()
        fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(10, 9))

        # Line styles / colors
        (ln_alt,) = ax1.plot([], [], linewidth=2.0, label="Alt", color="#2D7DD2")  # blue
        (ln_path,) = ax2.plot([], [], linewidth=2.0, label="Path", color="#F46036", marker='o', markersize=3)  # orange path
        (ln_current,) = ax2.plot([], [], 'ro', markersize=10, label="Current", zorder=5)  # red current position
        (ln_acc,) = ax3.plot([], [], linewidth=2.0, label="Acc", color="#44AF69")  # green

        # Labels
        ax1.set_ylabel(f"Alt ({UNIT_ALT})")
        ax1.set_xlabel("Time (s)")
        
        ax2.set_ylabel("Latitude (°)")
        ax2.set_xlabel("Longitude (°)")
        ax2.set_title("GPS Path")
        ax2.grid(True, alpha=0.3)
        ax2.set_aspect('equal', adjustable='box')
        
        ax3.set_ylabel(f"Acc ({UNIT_ACC})")
        ax3.set_xlabel("Time (s)")

        # Grid for time-based graphs
        ax1.grid(True, alpha=0.3)
        ax3.grid(True, alpha=0.3)

        # "current value" badges in each subplot (top-right)
        badge = dict(fontsize=9,
                     bbox=dict(boxstyle="round,pad=0.3", facecolor="white", alpha=0.8, edgecolor="#ddd"))
        ann_alt = ax1.text(0.98, 0.92, "", transform=ax1.transAxes, ha="right", va="top", **badge)
        ann_latlon = ax2.text(0.98, 0.92, "", transform=ax2.transAxes, ha="right", va="top", **badge)
        ann_acc = ax3.text(0.98, 0.92, "", transform=ax3.transAxes, ha="right", va="top", **badge)

        plt.tight_layout()
        plt.show(block=False)

        running = {"open": True}
        def _on_close(_evt):
            running["open"] = False
        fig.canvas.mpl_connect("close_event", _on_close)

        updater = asyncio.create_task(
            _plot_updater(fig, (ax1, ax2, ax3),
                          (ln_alt, ln_path, ln_current, ln_acc),
                          (ann_alt, ann_latlon, ann_acc),
                          WINDOW_SECS, running)
        )

        try:
            while running["open"]:
                await asyncio.sleep(0.2)
        finally:
            updater.cancel()
            with contextlib.suppress(Exception):
                await client.stop_notify(NUS_NOTIFY_UUID)

if __name__ == "__main__":
    try:
        asyncio.run(main())
    finally:
        try: log_file.close()
        except: pass
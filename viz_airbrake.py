import asyncio, csv, time, io, sys
from collections import deque
from bleak import BleakScanner, BleakClient
import matplotlib.pyplot as plt
import matplotlib.animation as animation

DEVICE_NAME = "ESP32-NUS-3"  # your advertised name
NUS_NOTIFY_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"  # TX notify
LINE_PREFIX = "TELEM/"

# Rolling buffers (~20s at 50 Hz; adjust as needed)
WINDOW_SECS = 180  # 3 minutes
ASSUMED_RATE_HZ = 2
MAX_POINTS = WINDOW_SECS * ASSUMED_RATE_HZ

t = deque(maxlen=MAX_POINTS)
alt_m = deque(maxlen=MAX_POINTS)
lat = deque(maxlen=MAX_POINTS)
lon = deque(maxlen=MAX_POINTS)
acc_z = deque(maxlen=MAX_POINTS)  # Vertical acceleration
vel_z = deque(maxlen=MAX_POINTS)  # Vertical velocity
state_pz = deque(maxlen=MAX_POINTS)  # State position Z
motor_angle = deque(maxlen=MAX_POINTS)  # Motor angle
flight_stage = deque(maxlen=MAX_POINTS)  # Flight stage
motor_bat_v = deque(maxlen=MAX_POINTS)  # Motor battery voltage
comp_bat_v = deque(maxlen=MAX_POINTS)  # Computer battery voltage

# CSV logger - save raw telemetry lines
log_path = f"telemetry_{int(time.time())}.csv"
log_file = open(log_path, "w", newline="")

# Notification line-assembler
_line_buf = bytearray()

def _parse_line(line: str):
    # New header format:
    # 0=time, 1=stage, 2=px, 3=py, 4=pz, 5=vx, 6=vy, 7=vz, 8=ax, 9=ay, 10=az, 11=apogee, 12=time_to_apogee, 13=off_vert, 14=time_in_stage
    # 15=baro_pres, 16=baro_temp, 17=baro_alt
    # 18=gps_lat, 19=gps_lon, 20=gps_alt, 21=gps_vN, 22=gps_vE, 23=gps_vD, 24=gps_fix, 25=gps_time
    # 26=imu_acc_x, 27=imu_acc_y, 28=imu_acc_z, 29=imu_gyro_x, 30=imu_gyro_y, 31=imu_gyro_z
    # 32=motor_pos, 33=motor_angle, 34=motor_vel, 35=motor_battery, 36=bat_voltage
    if not line.startswith(LINE_PREFIX):
        return None
    try:
        payload = line[len(LINE_PREFIX):]
        parts = payload.split(",")

        # Debug: print first parse attempt
        if len(parts) >= 20:
            print(f"[DEBUG] Parsing {len(parts)} columns: time={parts[0]}, stage={parts[1]}, pz={parts[4]}, vz={parts[7]}, az={parts[10]}")

        # Warn about unexpected column counts
        if len(parts) < 37 and len(parts) >= 20:
            print(f"[WARNING] Expected 37 columns, got {len(parts)} - telemetry format may be incomplete")

        if len(parts) < 20:  # need at least 20 columns
            print(f"[DEBUG] Skipping: only {len(parts)} columns")
            return None

        ts = float(parts[0])          # Time (s)
        stage = int(parts[1])         # Flight Stage
        state_pz = float(parts[4])    # State Position Z (m)
        vel_z_val = float(parts[7])   # State Velocity Z (m/s)
        acc_z_val = float(parts[10])  # State Acceleration Z (m/s^2)

        # Try column 17 for barometer alt, but if it's 0, use GPS alt from column 20
        baro_alt = float(parts[17])   # MS5611 Alt ASL (m)
        if baro_alt == 0.0 and len(parts) > 20:
            baro_alt = float(parts[20])  # Fall back to GPS altitude
            print(f"[DEBUG] Using GPS alt {baro_alt} instead of baro (was 0)")

        latitude = float(parts[18])   # GPS Lat (deg)
        longitude = float(parts[19])  # GPS Lon (deg)

        # Motor angle (column 33)
        motor_angle = float(parts[33]) if len(parts) > 33 else 0.0

        # Battery voltages (columns 35 and 36)
        motor_bat_voltage = float(parts[35]) if len(parts) > 35 else 0.0
        comp_bat_voltage = float(parts[36]) if len(parts) > 36 else 0.0

        return ts, stage, baro_alt, latitude, longitude, acc_z_val, vel_z_val, state_pz, motor_angle, motor_bat_voltage, comp_bat_voltage
    except (ValueError, IndexError) as e:
        # Debug parse errors
        print(f"[DEBUG] Parse error: {e} on line: {line[:100]}...")
        return None

def _on_notify(_, data: bytes):
    global _line_buf
    print(f"[BLE RX] Received {len(data)} bytes")  # Debug: see if we're getting data
    _line_buf.extend(data)
    # split by newline (ESP sends '\n'; handles '\r\n' too)
    *lines, remainder = _line_buf.split(b"\n")
    _line_buf = bytearray(remainder)
    for raw in lines:
        ln = raw.decode("utf-8", errors="ignore").strip("\r")
        # Only process complete lines (skip fragments from initial connection)
        if len(ln) < 10:  # Skip tiny fragments
            print(f"[BLE] Skipping fragment: {ln[:20]}")
            continue
        print(f"[BLE LINE] {ln}...")  # Show first 80 chars

        # Save complete raw telemetry line to CSV (if it's a TELEM line)
        if ln.startswith(LINE_PREFIX):
            log_file.write(ln[len(LINE_PREFIX):] + "\n")
            log_file.flush()

        parsed = _parse_line(ln)
        if parsed:
            ts, stage, alt, latitude, longitude, az, vz, pz, motor, motor_bat, comp_bat = parsed
            print(f"[PARSED OK] t={ts:.2f} stage={stage} alt={alt:.2f} pz={pz:.2f} vz={vz:.2f} az={az:.2f} motor={motor:.1f}° mbat={motor_bat:.2f}V cbat={comp_bat:.2f}V")
            t.append(ts)
            flight_stage.append(stage)
            alt_m.append(alt)
            lat.append(latitude)
            lon.append(longitude)
            acc_z.append(az)
            vel_z.append(vz)
            state_pz.append(pz)
            motor_angle.append(motor)
            motor_bat_v.append(motor_bat)
            comp_bat_v.append(comp_bat)

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

# Optional unit labels (edit to taste)
UNIT_ALT = "m"
UNIT_VEL = "m/s"
UNIT_ACC = "m/s²"
UNIT_MOTOR = "deg"

# Flight stage names
STAGE_NAMES = {
    0: "IDLE",
    1: "PAD",
    2: "BOOST",
    3: "COAST",
    4: "APOGEE",
    5: "DROGUE",
    6: "MAIN",
    7: "LANDED"
}

async def _plot_updater(fig, axes, lines, anns, status_overlay, window_secs, running_flag):
    ax1, ax2, ax3, ax4, ax5 = axes
    ln_alt_baro, ln_alt_state, ln_path, ln_current, ln_vel, ln_acc, ln_motor = lines
    ann_alt, ann_latlon, ann_vel, ann_acc, ann_motor = anns

    while running_flag["open"]:
        if t:
            t0 = t[0]
            xs = [ti - t0 for ti in t]
            y_alt_baro = list(alt_m)
            y_alt_state = list(state_pz)
            y_lat = list(lat)
            y_lon = list(lon)
            y_vel = list(vel_z)
            y_acc = list(acc_z)
            y_motor = list(motor_angle)

            # Altitude graph (Baro + State PZ overlay)
            ln_alt_baro.set_data(xs, y_alt_baro)
            ln_alt_state.set_data(xs, y_alt_state)

            # GPS path on map
            ln_path.set_data(y_lon, y_lat)
            # Current position marker
            if y_lon and y_lat:
                ln_current.set_data([y_lon[-1]], [y_lat[-1]])

            # Velocity graph
            ln_vel.set_data(xs, y_vel)

            # Acceleration graph
            ln_acc.set_data(xs, y_acc)

            # Motor angle graph
            ln_motor.set_data(xs, y_motor)

            # keep a 60s window (or your WINDOW_SECS) for time-based graphs
            xmax = xs[-1] if xs[-1] > 5 else window_secs
            xmin = max(0, xmax - window_secs)
            ax1.set_xlim(xmin, xmax)
            ax3.set_xlim(xmin, xmax)
            ax4.set_xlim(xmin, xmax)
            ax5.set_xlim(xmin, xmax)

            # autoscale Y-axis for time-series plots based on visible data
            # Altitude (ax1) - consider both baro and state data
            if xs and (y_alt_baro or y_alt_state):
                all_alt_vals = y_alt_baro + y_alt_state
                ax1.set_ylim(min(all_alt_vals) - 5, max(all_alt_vals) + 5)

            # Velocity (ax3)
            if xs and y_vel:
                ax3.set_ylim(min(y_vel) - 5, max(y_vel) + 5)

            # Acceleration (ax4)
            if xs and y_acc:
                ax4.set_ylim(min(y_acc) - 5, max(y_acc) + 5)

            # Motor angle (ax5)
            if xs and y_motor:
                ax5.set_ylim(min(y_motor) - 10, max(y_motor) + 10)

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
            ann_alt.set_text(f"Baro:{y_alt_baro[-1]:.1f} State:{y_alt_state[-1]:.1f} {UNIT_ALT}".strip())
            ann_latlon.set_text(f"{y_lat[-1]:.6f}°, {y_lon[-1]:.6f}°".strip())
            ann_vel.set_text(f"{y_vel[-1]:.2f} {UNIT_VEL}".strip())
            ann_acc.set_text(f"{y_acc[-1]:.2f} {UNIT_ACC}".strip())
            ann_motor.set_text(f"{y_motor[-1]:.1f} {UNIT_MOTOR}".strip())

            # Update status overlay (stage, time, battery)
            current_stage = list(flight_stage)[-1] if flight_stage else 0
            current_time = t[-1] if t else 0.0
            current_motor_bat = list(motor_bat_v)[-1] if motor_bat_v else 0.0
            current_comp_bat = list(comp_bat_v)[-1] if comp_bat_v else 0.0
            stage_name = STAGE_NAMES.get(current_stage, f"UNKNOWN({current_stage})")
            status_overlay.set_text(
                f"Stage: {stage_name} | FC Time: {current_time:.1f}s | Motor Bat: {current_motor_bat:.2f}V | Comp Bat: {current_comp_bat:.2f}V"
            )

            fig.canvas.draw_idle()
            plt.pause(0.001)  # let GUI process events

        await asyncio.sleep(0.1)  # ~10 Hz UI refresh

async def main():
    print(f"[INIT] Searching for BLE device '{DEVICE_NAME}'...")
    dev = await _find_device(DEVICE_NAME)
    if not dev:
        print(f"[ERROR] Device '{DEVICE_NAME}' not found.")
        return

    print(f"[INIT] Found device: {dev.address}")
    print(f"[INIT] Connecting...")

    async with BleakClient(dev) as client:
        if not client.is_connected:
            print("[ERROR] BLE connect failed.")
            return

        print(f"[INIT] Connected! Starting notifications...")
        await client.start_notify(NUS_NOTIFY_UUID, _on_notify)
        print(f"[INIT] Notifications started. Waiting for data...")

        # --- 5 stacked subplots: Alt, Map, Vel, Acc, Motor ---
        plt.ion()
        fig, (ax1, ax2, ax3, ax4, ax5) = plt.subplots(5, 1, figsize=(12, 16))

        # Prevent window from staying on top
        try:
            fig.canvas.manager.window.attributes('-topmost', False)
            fig.canvas.manager.window.lower()
        except:
            pass

        # Line styles / colors
        (ln_alt_baro,) = ax1.plot([], [], linewidth=2.0, label="Baro Alt", color="#2D7DD2")  # blue
        (ln_alt_state,) = ax1.plot([], [], linewidth=2.0, label="State PZ", color="#FF6B6B", linestyle='--')  # red dashed
        (ln_path,) = ax2.plot([], [], linewidth=2.0, label="Path", color="#F46036", marker='o', markersize=3)  # orange path
        (ln_current,) = ax2.plot([], [], 'ro', markersize=10, label="Current", zorder=5)  # red current position
        (ln_vel,) = ax3.plot([], [], linewidth=2.0, label="Vel Z", color="#9B59B6")  # purple
        (ln_acc,) = ax4.plot([], [], linewidth=2.0, label="Acc Z", color="#44AF69")  # green
        (ln_motor,) = ax5.plot([], [], linewidth=2.0, label="Motor", color="#E67E22")  # orange

        # Labels
        ax1.set_ylabel(f"Altitude ({UNIT_ALT})")
        ax1.legend(loc="upper left", frameon=False, fontsize=8)

        ax2.set_ylabel("Latitude (°)")
        ax2.set_xlabel("Longitude (°)")
        ax2.set_title("GPS Path")
        ax2.grid(True, alpha=0.3)
        ax2.set_aspect('equal', adjustable='box')

        ax3.set_ylabel(f"Vel Z ({UNIT_VEL})")

        ax4.set_ylabel(f"Acc Z ({UNIT_ACC})")

        ax5.set_ylabel(f"Motor ({UNIT_MOTOR})")
        ax5.set_xlabel("Time (s)")

        # Grid for time-based graphs
        for ax in (ax1, ax3, ax4, ax5):
            ax.grid(True, alpha=0.3)

        # "current value" badges in each subplot (top-right)
        badge = dict(fontsize=8,
                     bbox=dict(boxstyle="round,pad=0.3", facecolor="white", alpha=0.8, edgecolor="#ddd"))
        ann_alt = ax1.text(0.98, 0.92, "", transform=ax1.transAxes, ha="right", va="top", **badge)
        ann_latlon = ax2.text(0.98, 0.92, "", transform=ax2.transAxes, ha="right", va="top", **badge)
        ann_vel = ax3.text(0.98, 0.92, "", transform=ax3.transAxes, ha="right", va="top", **badge)
        ann_acc = ax4.text(0.98, 0.92, "", transform=ax4.transAxes, ha="right", va="top", **badge)
        ann_motor = ax5.text(0.98, 0.92, "", transform=ax5.transAxes, ha="right", va="top", **badge)

        # Status overlay at top of figure (stage, time, battery)
        status_overlay = fig.text(0.5, 0.98, "", ha="center", va="top", fontsize=12, fontweight="bold",
                                  bbox=dict(boxstyle="round,pad=0.5", facecolor="yellow", alpha=0.9, edgecolor="black"))

        plt.tight_layout()
        plt.show(block=False)

        running = {"open": True}
        def _on_close(_evt):
            running["open"] = False
        fig.canvas.mpl_connect("close_event", _on_close)

        updater = asyncio.create_task(
            _plot_updater(fig, (ax1, ax2, ax3, ax4, ax5),
                          (ln_alt_baro, ln_alt_state, ln_path, ln_current, ln_vel, ln_acc, ln_motor),
                          (ann_alt, ann_latlon, ann_vel, ann_acc, ann_motor),
                          status_overlay,
                          WINDOW_SECS, running)
        )

        try:
            while running["open"]:
                await asyncio.sleep(0.2)
        except (KeyboardInterrupt, asyncio.CancelledError):
            pass
        finally:
            print("\n[EXIT] Cleaning up...")
            updater.cancel()
            with contextlib.suppress(asyncio.CancelledError, Exception):
                await updater  # Wait for updater to finish
            with contextlib.suppress(Exception):
                await client.stop_notify(NUS_NOTIFY_UUID)
            print("[EXIT] Done")

if __name__ == "__main__":
    try:
        asyncio.run(main())
    finally:
        try: log_file.close()
        except: pass
"""Matplotlib dashboard for live AVITELEM and BPPTELEM telemetry."""

from __future__ import annotations

import asyncio
import math

import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D  # noqa: F401

from .config import UI_MAX_HZ, WINDOW_SECS
from .state import TelemetryHistory, latest_finite


def _finite(values: list[float]) -> list[float]:
    return [value for value in values if math.isfinite(value)]


def _set_ylim(ax, values: list[float], default_span: float, pad_frac: float = 0.1) -> None:
    finite_values = _finite(values)
    if not finite_values:
        return

    y_min = min(finite_values)
    y_max = max(finite_values)
    span = y_max - y_min

    if span < default_span:
        mid = 0.5 * (y_min + y_max)
        half = 0.5 * default_span
        ax.set_ylim(mid - half, mid + half)
        return

    pad = max(default_span * pad_frac, span * pad_frac)
    ax.set_ylim(y_min - pad, y_max + pad)


def _set_time_window(ax, xs: list[float]) -> None:
    xmax = xs[-1] if xs and xs[-1] > 5.0 else WINDOW_SECS
    xmin = max(0.0, xmax - WINDOW_SECS)
    ax.set_xlim(xmin, xmax)


def _set_gps_view(ax, longitudes: list[float], latitudes: list[float]) -> None:
    if not longitudes or not latitudes:
        return

    lon_min = min(longitudes)
    lon_max = max(longitudes)
    lat_min = min(latitudes)
    lat_max = max(latitudes)

    lon_range = max(lon_max - lon_min, 0.0001)
    lat_range = max(lat_max - lat_min, 0.0001)
    padding = 0.15

    ax.set_xlim(lon_min - (lon_range * padding), lon_max + (lon_range * padding))
    ax.set_ylim(lat_min - (lat_range * padding), lat_max + (lat_range * padding))


def _fmt_value(value: float, fmt: str, fallback: str = "--") -> str:
    if not math.isfinite(value):
        return fallback
    return format(value, fmt)


def _rotate_vector_by_quaternion(
    vector: tuple[float, float, float],
    w: float,
    x: float,
    y: float,
    z: float,
) -> tuple[float, float, float]:
    norm = math.sqrt((w * w) + (x * x) + (y * y) + (z * z))
    if norm <= 1e-9:
        return vector

    w /= norm
    x /= norm
    y /= norm
    z /= norm

    vx, vy, vz = vector
    tx = 2.0 * ((y * vz) - (z * vy))
    ty = 2.0 * ((z * vx) - (x * vz))
    tz = 2.0 * ((x * vy) - (y * vx))

    rx = vx + (w * tx) + ((y * tz) - (z * ty))
    ry = vy + (w * ty) + ((z * tx) - (x * tz))
    rz = vz + (w * tz) + ((x * ty) - (y * tx))
    return rx, ry, rz


class Dashboard:
    def __init__(self) -> None:
        plt.ion()
        self.fig = plt.figure(figsize=(14, 10), constrained_layout=True)
        grid = self.fig.add_gridspec(3, 2, height_ratios=[1.1, 1.0, 1.2], width_ratios=[1.0, 1.0])

        self.ax_alt = self.fig.add_subplot(grid[0, :])
        self.ax_vel = self.fig.add_subplot(grid[1, 0], sharex=self.ax_alt)
        self.ax_acc = self.fig.add_subplot(grid[1, 1], sharex=self.ax_alt)
        self.ax_gps = self.fig.add_subplot(grid[2, 0])
        self.ax_att = self.fig.add_subplot(grid[2, 1], projection="3d")

        self.fig.suptitle("STM32 Telemetry Viewer")

        (self.ln_alt,) = self.ax_alt.plot([], [], linewidth=2.0, color="#1f77b4", label="Primary Altitude")
        (self.ln_gps_alt,) = self.ax_alt.plot([], [], linewidth=1.8, color="#9467bd", linestyle="--", label="GPS Altitude")
        (self.ln_vel,) = self.ax_vel.plot([], [], linewidth=2.0, color="#ff7f0e", label="Primary Velocity")
        (self.ln_baro_vel,) = self.ax_vel.plot([], [], linewidth=1.8, color="#1f77b4", linestyle="--", label="Baro Velocity")
        (self.ln_gps_vel,) = self.ax_vel.plot([], [], linewidth=1.8, color="#9467bd", linestyle="--", label="GPS Velocity")
        (self.ln_acc,) = self.ax_acc.plot([], [], linewidth=2.0, color="#2ca02c", label="Vertical Accel")
        (self.ln_path,) = self.ax_gps.plot([], [], linewidth=2.0, color="#d62728", marker="o", markersize=3, label="GPS Path")
        (self.ln_current,) = self.ax_gps.plot([], [], marker="o", markersize=9, color="#111111", linestyle="", label="Current")
        (self.att_x_axis,) = self.ax_att.plot([], [], [], linewidth=3.0, color="#d62728", label="Forward (+X)")
        (self.att_y_axis,) = self.ax_att.plot([], [], [], linewidth=3.0, color="#2ca02c", label="Left (+Y)")
        (self.att_z_axis,) = self.ax_att.plot([], [], [], linewidth=3.0, color="#1f77b4", label="Up (+Z)")

        self.ax_alt.set_ylabel("Altitude (ft)")
        self.ax_alt.set_title("Altitude")
        self.ax_vel.set_ylabel("Vz (m/s)")
        self.ax_vel.set_xlabel("Time (s)")
        self.ax_vel.set_title("Velocity")
        self.ax_acc.set_ylabel("Az (m/s^2)")
        self.ax_acc.set_xlabel("Time (s)")
        self.ax_acc.set_title("Acceleration")
        self.ax_gps.set_title("GPS")
        self.ax_gps.set_xlabel("Longitude (deg)")
        self.ax_gps.set_ylabel("Latitude (deg)")
        self.ax_gps.set_aspect("equal", adjustable="box")
        self.ax_att.set_title("Orientation (ENU)")
        self.ax_att.set_xlabel("East")
        self.ax_att.set_ylabel("North")
        self.ax_att.set_zlabel("Up")
        self.ax_att.set_xlim(-1.1, 1.1)
        self.ax_att.set_ylim(-1.1, 1.1)
        self.ax_att.set_zlim(-1.1, 1.1)
        self.ax_att.set_box_aspect((1.0, 1.0, 1.0))
        self.ax_att.view_init(elev=20.0, azim=40.0)
        self.ax_att.plot([-1.0, 1.0], [0.0, 0.0], [0.0, 0.0], color="#bbbbbb", linewidth=1.0, alpha=0.7)
        self.ax_att.plot([0.0, 0.0], [-1.0, 1.0], [0.0, 0.0], color="#bbbbbb", linewidth=1.0, alpha=0.7)
        self.ax_att.plot([0.0, 0.0], [0.0, 0.0], [-1.0, 1.0], color="#bbbbbb", linewidth=1.0, alpha=0.7)
        self.ax_att.text(1.08, 0.0, 0.0, "E", color="#666666")
        self.ax_att.text(0.0, 1.08, 0.0, "N", color="#666666")
        self.ax_att.text(0.0, 0.0, 1.08, "U", color="#666666")

        for ax in (self.ax_alt, self.ax_vel, self.ax_acc):
            ax.grid(True, alpha=0.3)
            ax.legend(loc="upper left", frameon=False)
        self.ax_gps.grid(True, alpha=0.3)
        self.ax_gps.legend(loc="upper center", bbox_to_anchor=(0.5, -0.16), ncol=2, frameon=False)
        self.ax_att.grid(True, alpha=0.25)
        self.ax_att.legend(loc="upper center", bbox_to_anchor=(0.5, -0.16), ncol=3, frameon=False, fontsize=9)

        badge = dict(
            fontsize=9,
            bbox=dict(boxstyle="round,pad=0.25", facecolor="white", alpha=0.85, edgecolor="#dddddd"),
        )
        self.txt_alt = self.ax_alt.text(0.98, 0.9, "", transform=self.ax_alt.transAxes, ha="right", va="top", **badge)
        self.txt_vel = self.ax_vel.text(0.98, 0.9, "", transform=self.ax_vel.transAxes, ha="right", va="top", **badge)
        self.txt_acc = self.ax_acc.text(0.98, 0.9, "", transform=self.ax_acc.transAxes, ha="right", va="top", **badge)
        self.txt_gps = self.ax_gps.text(0.98, 0.95, "", transform=self.ax_gps.transAxes, ha="right", va="top", **badge)
        self.txt_att = self.ax_att.text2D(0.02, 0.95, "", transform=self.ax_att.transAxes, ha="left", va="top", **badge)
        self.status = self.fig.text(
            0.5,
            0.985,
            "",
            ha="center",
            va="top",
            fontsize=11,
            fontweight="bold",
            bbox=dict(boxstyle="round,pad=0.35", facecolor="#f7f7f7", alpha=0.95, edgecolor="#dddddd"),
        )

        plt.show(block=False)

    def bind_close(self, running: dict[str, bool]) -> None:
        def _on_close(_event) -> None:
            running["open"] = False

        self.fig.canvas.mpl_connect("close_event", _on_close)

    def update(self, history: TelemetryHistory) -> None:
        times = list(history.time_s)
        if not times:
            return

        t0 = times[0]
        xs = [value - t0 for value in times]

        altitude = list(history.altitude_ft)
        gps_altitude = list(history.gps_altitude_ft)
        velocity = list(history.velocity_z_ms)
        baro_velocity = list(history.baro_velocity_z_ms)
        gps_velocity = list(history.gps_velocity_z_ms)
        accel = list(history.accel_z_ms2)
        roll = list(history.roll_deg)
        pitch = list(history.pitch_deg)
        yaw = list(history.yaw_deg)
        quat_w = list(history.quat_w)
        quat_x = list(history.quat_x)
        quat_y = list(history.quat_y)
        quat_z = list(history.quat_z)

        self.ln_alt.set_data(xs, altitude)
        self.ln_gps_alt.set_data(xs, gps_altitude)
        self.ln_vel.set_data(xs, velocity)
        self.ln_baro_vel.set_data(xs, baro_velocity)
        self.ln_gps_vel.set_data(xs, gps_velocity)
        self.ln_acc.set_data(xs, accel)

        for ax in (self.ax_alt, self.ax_vel, self.ax_acc):
            _set_time_window(ax, xs)

        _set_ylim(self.ax_alt, altitude + gps_altitude, default_span=100.0)
        _set_ylim(self.ax_vel, velocity + baro_velocity + gps_velocity, default_span=20.0)
        _set_ylim(self.ax_acc, accel, default_span=20.0)

        latitudes = list(history.latitude_deg)
        longitudes = list(history.longitude_deg)
        gps_pairs = [
            (lon, lat)
            for lon, lat in zip(longitudes, latitudes)
            if math.isfinite(lon) and math.isfinite(lat)
        ]
        if gps_pairs:
            gps_lons = [lon for lon, _ in gps_pairs]
            gps_lats = [lat for _, lat in gps_pairs]
            self.ln_path.set_data(gps_lons, gps_lats)
            self.ln_current.set_data([gps_lons[-1]], [gps_lats[-1]])
            _set_gps_view(self.ax_gps, gps_lons, gps_lats)
            self.txt_gps.set_text(f"{gps_lats[-1]:.6f}, {gps_lons[-1]:.6f}")
        else:
            self.ln_path.set_data([], [])
            self.ln_current.set_data([], [])
            self.txt_gps.set_text("No GPS fix")

        latest_altitude = latest_finite(history.altitude_ft)
        latest_gps_altitude = latest_finite(history.gps_altitude_ft)
        latest_velocity = latest_finite(history.velocity_z_ms)
        latest_baro_velocity = latest_finite(history.baro_velocity_z_ms)
        latest_gps_velocity = latest_finite(history.gps_velocity_z_ms)
        latest_accel = latest_finite(history.accel_z_ms2)
        latest_roll = latest_finite(history.roll_deg)
        latest_pitch = latest_finite(history.pitch_deg)
        latest_yaw = latest_finite(history.yaw_deg)
        latest_qw = latest_finite(history.quat_w)
        latest_qx = latest_finite(history.quat_x)
        latest_qy = latest_finite(history.quat_y)
        latest_qz = latest_finite(history.quat_z)

        alt_text = _fmt_value(latest_altitude, ".1f")
        gps_alt_text = _fmt_value(latest_gps_altitude, ".1f")
        self.txt_alt.set_text(f"Primary {alt_text} ft | GPS {gps_alt_text} ft")
        self.txt_vel.set_text(f"{_fmt_value(latest_velocity, '.2f')} m/s")
        self.txt_acc.set_text(f"{_fmt_value(latest_accel, '.2f')} m/s^2")

        has_attitude = all(math.isfinite(value) for value in (latest_qw, latest_qx, latest_qy, latest_qz))
        if has_attitude:
            self.txt_att.set_text(
                f"ENU | R {_fmt_value(latest_roll, '.1f')} | P {_fmt_value(latest_pitch, '.1f')} | Y {_fmt_value(latest_yaw, '.1f')}"
            )
            axes = {
                self.att_x_axis: _rotate_vector_by_quaternion((1.0, 0.0, 0.0), latest_qw, latest_qx, latest_qy, latest_qz),
                self.att_y_axis: _rotate_vector_by_quaternion((0.0, 1.0, 0.0), latest_qw, latest_qx, latest_qy, latest_qz),
                self.att_z_axis: _rotate_vector_by_quaternion((0.0, 0.0, 1.0), latest_qw, latest_qx, latest_qy, latest_qz),
            }
            for line, (vx, vy, vz) in axes.items():
                line.set_data_3d([0.0, vx], [0.0, vy], [0.0, vz])
        else:
            self.txt_att.set_text("No attitude in current packet stream")
            for line, axis in (
                (self.att_x_axis, (1.0, 0.0, 0.0)),
                (self.att_y_axis, (0.0, 1.0, 0.0)),
                (self.att_z_axis, (0.0, 0.0, 1.0)),
            ):
                vx, vy, vz = axis
                line.set_data_3d([0.0, vx], [0.0, vy], [0.0, vz])

        latest_battery = latest_finite(history.battery_volts)
        latest_temperature = latest_finite(history.temperature_c)
        battery_text = "--" if not math.isfinite(latest_battery) else f"{latest_battery:.2f} V"
        temperature_text = "--" if not math.isfinite(latest_temperature) else f"{latest_temperature:.1f} C"
        gps_text = "FIX" if gps_pairs else "NO FIX"
        self.status.set_text(
            f"Samples: {len(times)} | Alt {_fmt_value(latest_altitude, '.1f')} ft | "
            f"GPS Alt {_fmt_value(latest_gps_altitude, '.1f')} ft | "
            f"Vz {_fmt_value(latest_velocity, '.2f')} m/s | "
            f"Az {_fmt_value(latest_accel, '.2f')} m/s^2 | Battery {battery_text} | Temp {temperature_text} | GPS {gps_text}"
        )

        self.fig.canvas.draw_idle()
        plt.pause(0.001)

    async def run(self, history: TelemetryHistory, running: dict[str, bool]) -> None:
        last_draw_t = 0.0
        last_len = 0

        while running["open"]:
            if history.is_empty():
                plt.pause(0.001)
                await asyncio.sleep(0.05)
                continue

            now = asyncio.get_running_loop().time()
            current_len = len(history.time_s)
            if current_len == last_len or (now - last_draw_t) < (1.0 / UI_MAX_HZ):
                plt.pause(0.001)
                await asyncio.sleep(0.01)
                continue

            last_len = current_len
            last_draw_t = now
            self.update(history)
            await asyncio.sleep(0.001)

    def close(self) -> None:
        plt.close(self.fig)

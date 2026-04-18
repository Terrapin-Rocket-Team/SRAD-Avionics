"""Rolling in-memory telemetry history."""

from __future__ import annotations

import math
from collections import deque
from dataclasses import dataclass, field

from .config import MAX_POINTS
from .protocol import TelemetrySample


def latest_finite(values: deque[float]) -> float:
    for value in reversed(values):
        if math.isfinite(value):
            return value
    return math.nan


@dataclass(slots=True)
class TelemetryHistory:
    max_points: int = MAX_POINTS
    time_s: deque[float] = field(init=False)
    altitude_ft: deque[float] = field(init=False)
    gps_altitude_ft: deque[float] = field(init=False)
    velocity_z_ms: deque[float] = field(init=False)
    baro_velocity_z_ms: deque[float] = field(init=False)
    gps_velocity_z_ms: deque[float] = field(init=False)
    accel_z_ms2: deque[float] = field(init=False)
    quat_w: deque[float] = field(init=False)
    quat_x: deque[float] = field(init=False)
    quat_y: deque[float] = field(init=False)
    quat_z: deque[float] = field(init=False)
    roll_deg: deque[float] = field(init=False)
    pitch_deg: deque[float] = field(init=False)
    yaw_deg: deque[float] = field(init=False)
    battery_volts: deque[float] = field(init=False)
    temperature_c: deque[float] = field(init=False)
    latitude_deg: deque[float] = field(init=False)
    longitude_deg: deque[float] = field(init=False)

    def __post_init__(self) -> None:
        self.time_s = deque(maxlen=self.max_points)
        self.altitude_ft = deque(maxlen=self.max_points)
        self.gps_altitude_ft = deque(maxlen=self.max_points)
        self.velocity_z_ms = deque(maxlen=self.max_points)
        self.baro_velocity_z_ms = deque(maxlen=self.max_points)
        self.gps_velocity_z_ms = deque(maxlen=self.max_points)
        self.accel_z_ms2 = deque(maxlen=self.max_points)
        self.quat_w = deque(maxlen=self.max_points)
        self.quat_x = deque(maxlen=self.max_points)
        self.quat_y = deque(maxlen=self.max_points)
        self.quat_z = deque(maxlen=self.max_points)
        self.roll_deg = deque(maxlen=self.max_points)
        self.pitch_deg = deque(maxlen=self.max_points)
        self.yaw_deg = deque(maxlen=self.max_points)
        self.battery_volts = deque(maxlen=self.max_points)
        self.temperature_c = deque(maxlen=self.max_points)
        self.latitude_deg = deque(maxlen=self.max_points)
        self.longitude_deg = deque(maxlen=self.max_points)

    def append(self, sample: TelemetrySample) -> None:
        self.time_s.append(sample.time_s)
        self.altitude_ft.append(sample.altitude_ft)
        self.gps_altitude_ft.append(sample.gps_altitude_ft)
        self.velocity_z_ms.append(sample.velocity_z_ms)
        self.baro_velocity_z_ms.append(sample.baro_velocity_z_ms)
        self.gps_velocity_z_ms.append(sample.gps_velocity_z_ms)
        self.accel_z_ms2.append(sample.accel_z_ms2)
        self.quat_w.append(sample.quat_w)
        self.quat_x.append(sample.quat_x)
        self.quat_y.append(sample.quat_y)
        self.quat_z.append(sample.quat_z)
        self.roll_deg.append(sample.roll_deg)
        self.pitch_deg.append(sample.pitch_deg)
        self.yaw_deg.append(sample.yaw_deg)
        self.battery_volts.append(sample.battery_volts)
        self.temperature_c.append(sample.temperature_c)
        self.latitude_deg.append(sample.latitude_deg)
        self.longitude_deg.append(sample.longitude_deg)

    def is_empty(self) -> bool:
        return not self.time_s

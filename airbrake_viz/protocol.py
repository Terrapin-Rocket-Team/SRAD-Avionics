"""Packet decoding for the STM32 AVITELEM and BPPTELEM streams."""

from __future__ import annotations

import math
import time
from dataclasses import dataclass
from enum import IntEnum


class MessageType(IntEnum):
    ABTELEM = 0
    AVITELEM = 1
    ABCMD = 2
    AVICMD = 3
    LIVECMD = 4
    NOSEVIDCMD = 5
    ABVIDCMD = 6
    BPPTELEM = 7


# Keep this aligned with STM32FC/src/AvionicsPacketProtocol.h::aviTelemetryPayloadSize().
AVI_TELEMETRY_PAYLOAD_SIZE = 26
BPP_TELEMETRY_PAYLOAD_SIZE = 25
FIXED_PAYLOAD_SIZES = {
    MessageType.AVITELEM: {AVI_TELEMETRY_PAYLOAD_SIZE},
    MessageType.BPPTELEM: {BPP_TELEMETRY_PAYLOAD_SIZE},
}
UNKNOWN_GPS_E7 = -(2**31)
UNKNOWN_SIGNED16 = -(2**15)
UNKNOWN_SIGNED32 = -(2**31)


@dataclass(slots=True)
class TelemetrySample:
    time_s: float
    altitude_ft: float
    gps_altitude_ft: float
    velocity_z_ms: float
    baro_velocity_z_ms: float
    gps_velocity_z_ms: float
    accel_z_ms2: float
    baro_agl_ft: float
    quat_w: float
    quat_x: float
    quat_y: float
    quat_z: float
    roll_deg: float
    pitch_deg: float
    yaw_deg: float
    battery_volts: float
    temperature_c: float
    latitude_deg: float
    longitude_deg: float


@dataclass(slots=True)
class StreamDebugStats:
    chunks_in: int = 0
    bytes_in: int = 0
    packets_framed: int = 0
    invalid_bytes_dropped: int = 0
    avitelem_packets: int = 0
    bpptelem_packets: int = 0
    zero_payload_packets: int = 0
    empty_samples: int = 0
    signal_samples: int = 0
    last_packet_hex: str = ""
    last_sample_summary: str = ""


def _read_u16le(payload: bytes, offset: int) -> int:
    return int.from_bytes(payload[offset : offset + 2], "little", signed=False)


def _read_i16le(payload: bytes, offset: int) -> int:
    return int.from_bytes(payload[offset : offset + 2], "little", signed=True)


def _read_i32le(payload: bytes, offset: int) -> int:
    return int.from_bytes(payload[offset : offset + 4], "little", signed=True)


def _normalize_quaternion(w: float, x: float, y: float, z: float) -> tuple[float, float, float, float]:
    norm = math.sqrt((w * w) + (x * x) + (y * y) + (z * z))
    if norm <= 1e-9:
        return 1.0, 0.0, 0.0, 0.0
    return w / norm, x / norm, y / norm, z / norm


def quaternion_to_euler_deg(w: float, x: float, y: float, z: float) -> tuple[float, float, float]:
    w, x, y, z = _normalize_quaternion(w, x, y, z)

    sinr_cosp = 2.0 * ((w * x) + (y * z))
    cosr_cosp = 1.0 - (2.0 * ((x * x) + (y * y)))
    roll = math.atan2(sinr_cosp, cosr_cosp)

    sinp = 2.0 * ((w * y) - (z * x))
    if abs(sinp) >= 1.0:
        pitch = math.copysign(math.pi / 2.0, sinp)
    else:
        pitch = math.asin(sinp)

    siny_cosp = 2.0 * ((w * z) + (x * y))
    cosy_cosp = 1.0 - (2.0 * ((y * y) + (z * z)))
    yaw = math.atan2(siny_cosp, cosy_cosp)

    return math.degrees(roll), math.degrees(pitch), math.degrees(yaw)


def decode_avi_telemetry(packet: bytes, sample_time_s: float) -> TelemetrySample:
    payload = packet[2:]

    altitude_ft = float(_read_i16le(payload, 0))
    velocity_z_ms = float(_read_i16le(payload, 2)) / 10.0
    accel_z_ms2 = float(_read_i16le(payload, 4)) / 10.0
    baro_agl_raw = _read_i16le(payload, 6)
    baro_agl_ft = math.nan if baro_agl_raw == UNKNOWN_SIGNED16 else float(baro_agl_raw)

    quat_w = float(_read_i16le(payload, 8)) / 32767.0
    quat_x = float(_read_i16le(payload, 10)) / 32767.0
    quat_y = float(_read_i16le(payload, 12)) / 32767.0
    quat_z = float(_read_i16le(payload, 14)) / 32767.0
    roll_deg, pitch_deg, yaw_deg = quaternion_to_euler_deg(quat_w, quat_x, quat_y, quat_z)

    battery_centivolts = _read_u16le(payload, 16)
    battery_volts = math.nan if battery_centivolts == 0xFFFF else battery_centivolts / 100.0

    latitude_e7 = _read_i32le(payload, 18)
    longitude_e7 = _read_i32le(payload, 22)
    latitude_deg = math.nan if latitude_e7 == UNKNOWN_GPS_E7 else latitude_e7 / 10_000_000.0
    longitude_deg = math.nan if longitude_e7 == UNKNOWN_GPS_E7 else longitude_e7 / 10_000_000.0

    return TelemetrySample(
        time_s=sample_time_s,
        altitude_ft=altitude_ft,
        gps_altitude_ft=math.nan,
        velocity_z_ms=velocity_z_ms,
        baro_velocity_z_ms=math.nan,
        gps_velocity_z_ms=math.nan,
        accel_z_ms2=accel_z_ms2,
        baro_agl_ft=baro_agl_ft,
        quat_w=quat_w,
        quat_x=quat_x,
        quat_y=quat_y,
        quat_z=quat_z,
        roll_deg=roll_deg,
        pitch_deg=pitch_deg,
        yaw_deg=yaw_deg,
        battery_volts=battery_volts,
        temperature_c=math.nan,
        latitude_deg=latitude_deg,
        longitude_deg=longitude_deg,
    )


def decode_bpp_telemetry(packet: bytes, sample_time_s: float) -> TelemetrySample:
    payload = packet[2:]

    flags = payload[0]

    baro_altitude_raw = _read_i32le(payload, 1)
    baro_altitude_ft = math.nan
    if (flags & 0x01) and baro_altitude_raw != UNKNOWN_SIGNED32:
        baro_altitude_ft = float(baro_altitude_raw)

    gps_altitude_raw = _read_i32le(payload, 5)
    gps_altitude_ft = math.nan
    if (flags & 0x02) and gps_altitude_raw != UNKNOWN_SIGNED32:
        gps_altitude_ft = float(gps_altitude_raw)

    battery_centivolts = _read_u16le(payload, 9)
    battery_volts = math.nan
    if (flags & 0x04) and battery_centivolts != 0xFFFF:
        battery_volts = battery_centivolts / 100.0

    latitude_e7 = _read_i32le(payload, 11)
    longitude_e7 = _read_i32le(payload, 15)
    latitude_deg = math.nan
    longitude_deg = math.nan
    if (flags & 0x02) and latitude_e7 != UNKNOWN_GPS_E7 and longitude_e7 != UNKNOWN_GPS_E7:
        latitude_deg = latitude_e7 / 10_000_000.0
        longitude_deg = longitude_e7 / 10_000_000.0

    baro_velocity_raw = _read_i16le(payload, 19)
    gps_velocity_raw = _read_i16le(payload, 21)
    baro_temperature_raw = _read_i16le(payload, 23)
    baro_velocity_z_ms = math.nan
    gps_velocity_z_ms = math.nan
    temperature_c = math.nan
    if (flags & 0x08) and baro_velocity_raw != UNKNOWN_SIGNED16:
        baro_velocity_z_ms = baro_velocity_raw / 10.0
    if (flags & 0x10) and gps_velocity_raw != UNKNOWN_SIGNED16:
        gps_velocity_z_ms = gps_velocity_raw / 10.0
    if (flags & 0x20) and baro_temperature_raw != UNKNOWN_SIGNED16:
        temperature_c = baro_temperature_raw / 10.0

    return TelemetrySample(
        time_s=sample_time_s,
        altitude_ft=baro_altitude_ft,
        gps_altitude_ft=gps_altitude_ft,
        velocity_z_ms=math.nan,
        baro_velocity_z_ms=baro_velocity_z_ms,
        gps_velocity_z_ms=gps_velocity_z_ms,
        accel_z_ms2=math.nan,
        baro_agl_ft=baro_altitude_ft,
        quat_w=math.nan,
        quat_x=math.nan,
        quat_y=math.nan,
        quat_z=math.nan,
        roll_deg=math.nan,
        pitch_deg=math.nan,
        yaw_deg=math.nan,
        battery_volts=battery_volts,
        temperature_c=temperature_c,
        latitude_deg=latitude_deg,
        longitude_deg=longitude_deg,
    )


def sample_has_signal(sample: TelemetrySample) -> bool:
    numeric_values = (
        sample.altitude_ft,
        sample.gps_altitude_ft,
        sample.velocity_z_ms,
        sample.baro_velocity_z_ms,
        sample.gps_velocity_z_ms,
        sample.accel_z_ms2,
        sample.baro_agl_ft,
        sample.battery_volts,
        sample.temperature_c,
        sample.latitude_deg,
        sample.longitude_deg,
    )
    return any(math.isfinite(value) and abs(value) > 1e-9 for value in numeric_values)


def summarize_sample(sample: TelemetrySample) -> str:
    baro_text = "nan" if not math.isfinite(sample.baro_agl_ft) else f"{sample.baro_agl_ft:.1f}ft"
    gps_alt_text = "nan" if not math.isfinite(sample.gps_altitude_ft) else f"{sample.gps_altitude_ft:.1f}ft"
    baro_vz_text = "nan" if not math.isfinite(sample.baro_velocity_z_ms) else f"{sample.baro_velocity_z_ms:.2f}m/s"
    gps_vz_text = "nan" if not math.isfinite(sample.gps_velocity_z_ms) else f"{sample.gps_velocity_z_ms:.2f}m/s"
    battery_text = "nan" if not math.isfinite(sample.battery_volts) else f"{sample.battery_volts:.2f}V"
    temp_text = "nan" if not math.isfinite(sample.temperature_c) else f"{sample.temperature_c:.1f}C"
    lat_text = "nan" if not math.isfinite(sample.latitude_deg) else f"{sample.latitude_deg:.6f}"
    lon_text = "nan" if not math.isfinite(sample.longitude_deg) else f"{sample.longitude_deg:.6f}"
    return (
        f"alt={sample.altitude_ft:.1f}ft vz={sample.velocity_z_ms:.2f}m/s "
        f"baro_vz={baro_vz_text} gps_vz={gps_vz_text} "
        f"az={sample.accel_z_ms2:.2f}m/s^2 baro={baro_text} gps_alt={gps_alt_text} batt={battery_text} temp={temp_text} "
        f"lat={lat_text} lon={lon_text} "
        f"quat=({sample.quat_w:.3f},{sample.quat_x:.3f},{sample.quat_y:.3f},{sample.quat_z:.3f})"
    )


class PacketStreamDecoder:
    """Assemble packet-framed telemetry from arbitrary BLE chunks."""

    def __init__(self, stats: StreamDebugStats) -> None:
        self._buffer = bytearray()
        self._stats = stats

    def feed(self, data: bytes) -> list[bytes]:
        if not data:
            return []

        self._stats.chunks_in += 1
        self._stats.bytes_in += len(data)
        self._buffer.extend(data)
        packets: list[bytes] = []

        while len(self._buffer) >= 2:
            raw_type = self._buffer[0]
            payload_len = self._buffer[1]

            try:
                msg_type = MessageType(raw_type)
            except ValueError:
                self._stats.invalid_bytes_dropped += 1
                del self._buffer[0]
                continue

            expected_payload_lens = FIXED_PAYLOAD_SIZES.get(msg_type)
            if expected_payload_lens is None or payload_len not in expected_payload_lens:
                self._stats.invalid_bytes_dropped += 1
                del self._buffer[0]
                continue

            packet_len = 2 + payload_len
            if len(self._buffer) < packet_len:
                break

            packets.append(bytes(self._buffer[:packet_len]))
            self._stats.packets_framed += 1
            del self._buffer[:packet_len]

        return packets


class AviTelemetryStreamParser:
    """Decode both legacy AVITELEM and newer BPPTELEM packets."""

    def __init__(self) -> None:
        self.stats = StreamDebugStats()
        self._decoder = PacketStreamDecoder(self.stats)
        self._host_t0 = time.monotonic()

    def feed(self, data: bytes) -> list[TelemetrySample]:
        samples: list[TelemetrySample] = []
        for packet in self._decoder.feed(data):
            msg_type = MessageType(packet[0])
            self.stats.last_packet_hex = packet.hex(" ")
            if all(byte == 0 for byte in packet[2:]):
                self.stats.zero_payload_packets += 1
            sample_time_s = time.monotonic() - self._host_t0

            if msg_type == MessageType.AVITELEM:
                self.stats.avitelem_packets += 1
                sample = decode_avi_telemetry(packet, sample_time_s)
            elif msg_type == MessageType.BPPTELEM:
                self.stats.bpptelem_packets += 1
                sample = decode_bpp_telemetry(packet, sample_time_s)
            else:
                continue

            self.stats.last_sample_summary = summarize_sample(sample)
            if sample_has_signal(sample):
                self.stats.signal_samples += 1
            else:
                self.stats.empty_samples += 1
            samples.append(sample)
        return samples

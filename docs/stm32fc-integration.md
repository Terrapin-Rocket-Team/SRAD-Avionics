# STM32FC integration reference

This page records what `Side_Projects/STM32FC` currently configures in source.
It is an implementation reference, not evidence that a PCB or assembled flight
computer has passed electrical or flight testing.

## Current target

- MCU/board target: custom STM32H723VEHX PlatformIO board definition
- Framework: STM32 Arduino, C++17
- Application layer: Astra-Rocket from its canonical `main` branch
- ARC node address: nose flight computer (`ARC_ADDR_FC_N`)
- Maintained compile check: `pio run` from `Side_Projects/STM32FC`

The project currently follows moving dependency branches. Pin reviewed release
commits or tags before freezing a mission build.

## Source pin assignments

These assignments come from `src/main.cpp` and `src/LocalFileCommands.cpp`.
Verify every signal against the schematic, PCB revision, assembled board, and
MCU alternate-function mapping before powering hardware or ordering boards.

| Function | Source assignment |
| --- | --- |
| Heartbeat LED | `PE5` |
| Telemetry LED | `PE6` |
| Telemetry UART TX / RX | `PB13` / `PB12` |
| Battery sense ADC | `PC2_C` |
| Radio reset / busy / chip select | `PC13` / `PE3` / `PA15` |
| Radio IO8 / IO9 (IRQ) | `PA3` / `PE2` |
| Radio MOSI / MISO / SCK | `PD7` / `PB4` / `PB3` |
| eMMC D0–D3 | `PC8`, `PC9`, `PC10`, `PC11` |
| eMMC clock / command | `PC12` / `PD2` |

Both diagnostic LEDs are currently treated as active-high. A successful ARC
heartbeat pulses `PE5`; successful telemetry/downlink sends pulse `PE6`. If
`AstraRocket::init()` fails, both LEDs fast-blink together and normal loop
processing never starts.

## Configured sensors and logging

| Role | Current implementation |
| --- | --- |
| IMU | `BMI088` |
| Barometer | `DPS368` |
| GPS | `SAM_M10Q` |
| Magnetometer | `MMC5603NJ` |
| High-G accelerometer | `H3LIS331DL` on `Wire`, address `0x19` |
| Battery monitor | `VoltageSensor`, 422 kΩ / 102 kΩ divider |

The battery telemetry applies a source-level calibration gain of `1.00733` and
offset of `-0.064 V`. Confirm those values against calibrated measurements for
the actual board.

The BMI088 mounting transform is `ROTATE_NEG90_Z` composed with `FLIP_YZ`. The
magnetometer is configured with the identity transform. These are source
assumptions that require an axis/sign test on the installed board.

Data telemetry is written to `data_log.csv` on eMMC. Events are written to
`event_log.csv` on eMMC and also to USB serial. Configured data-log rates are
50 Hz in flight and 10 Hz before and after flight. Astra's file sink may select
a unique suffixed filename when the requested name already exists.

## ARC and downlink path

`TelemetryUART` runs at 115200 baud and is the link to the ARC hub. The current
application sends:

- an ARC network-management heartbeat at 1 Hz;
- ARC flight telemetry addressed to ground at 10 Hz; and
- a vendor data-radio frame, wrapped in ARC `RADIO/DATA_DOWNLINK`, at 1 Hz.

The LR1121-related pins and driver source remain in the project, but `main.cpp`
currently routes the data-radio frame through the ARC hub. It does not use an
onboard radio for that downlink.

`ARC_DEBUG_MIRROR` is currently `1`, which copies raw outgoing ARC frames to USB
serial for diagnostics. The source explicitly says to set it to `0` for flight;
make that an item in the mission configuration review rather than assuming the
development default is flight-ready.

## Local eMMC access

After initialization, the telemetry UART is also registered with Astra's message
router. Newline-terminated local commands use the `FILE/` prefix:

| Command | Effect |
| --- | --- |
| `FILE/LS [path]` | Recursively list a path (root by default) |
| `FILE/CP <path>` | Stream one file between `FILE/BOF` and `FILE/EOF` markers |
| `FILE/RM <path>` | Recursively remove one path |
| `FILE/CLEAR [path]` | Recursively clear a path (root by default) |

`RM` and `CLEAR` are destructive. Recover and verify required logs before using
them, and exercise these commands on non-flight media during bring-up.

## What remains to verify

Compilation does not establish electrical correctness, sensor orientation,
storage integrity, ARC interoperability, stage behavior, or recovery-system
operation. Use the [bench and flight readiness checklist](validation-checklist.md)
and retain its evidence with the mission configuration.

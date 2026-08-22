# SRAD-Avionics 2026 consolidation status

Last verified: 2026-08-22

This document records repository-reconciliation decisions only. It is not a claim that unfinished hardware or firmware has been completed.

## Integration candidate

- Candidate branch: `codex/avionics-consolidation-no-jhauerst`
- Reviewed implementation commit (before this report): `8175f967`
- `main` remained at `90529e94` while this report was prepared.
- The candidate is a descendant of `main`; no force-push of `main` is required.

The candidate includes the current non-conflicting work from:

- `E22_breakout_board`
- `Jack-Yeulenski-FC2.0`
- `pcb-changes`
- `Macket`
- `STM32-Radio-Module`
- the project files from the sanitized `flight-computer` endpoint

Generated editor state, machine-local settings, a committed Python virtual environment, and redundant PCB backup copies were omitted. Deleted Astra and Astra-Rocket development-branch dependencies were changed to their canonical `main` branches.

## Preserved owner-controlled branches

The following jhauerst-owned lines were deliberately not merged or tagged. Leave them intact for their owner:

- `intra-rocket-video`
- `mid-teensy`
- `mpm-telemetry`
- `ARCH-Mega`

The candidate was explicitly checked and none of those four branch tips is reachable from it.

## Archived obsolete branch tips

These non-jhauerst branch tips were judged obsolete or superseded and were preserved with annotated tags before branch deletion:

- `267-flight-computer---sensors` -> `archive/branch-267-flight-computer-sensors-2025-10-09`
- `85-self-positioning-antenna-system-optional` -> `archive/branch-85-self-positioning-antenna-2025-09-15`
- `launch-7-12` -> `archive/launch-7-12-2025`
- `launch-jan` -> `archive/launch-jan-2026`
- `new_member_project` -> `archive/new-member-project-2026`
- `sdr_dev` -> `archive/sdr-dev-2025`
- `sdr-code-venv` -> `archive/sdr-code-venv-2025`
- `sdr-code` -> `archive/sdr-code-2025`

Three earlier Personal-fork tips are also retained as `archive/personal-*` tags.

## Build matrix

The repository's GitHub Actions workflow runs `platformio run` in every directory containing `platformio.ini`. The same matrix was run locally against the candidate.

Passing projects:

- `Side_Projects/ARC/RS-FEC`
- `Side_Projects/ESP32FC`
- `Side_Projects/Radio_Code` (all five environments)
- `Side_Projects/STM32FC`
- `Side_Projects/STM32FC-BPP`
- `Side_Projects/STM32FCTest`
- `Side_Projects/TeensyFCTest`
- `Side_Projects/Umbilical`
- `Spaceport/Code/ESP32-Bluetooth`
- `Spaceport/Code/STM32-Motor-Driver`

Known failing or mixed projects, intentionally left for future development:

- `Side_Projects/ARC/live-video-teensy`: current RadioMessage API mismatch.
- `Side_Projects/ESP32GS`: `esp32s3` passes; `ESP32GS` lacks the NimBLE dependency/source separation it needs.
- `Spaceport/Code/ARCH-Mega`: current STM32 `main` environment passes; legacy Teensy and Uno environments do not match the STM32 pin map. Its owner branch remains unmerged.
- `Spaceport/Code/Ground-Receiver`: both environments have source-level message/stream type errors.
- `Spaceport/Code/Kalman-Filter/Matrix-Benchmark`: missing BMP3XX build dependency.
- `Spaceport/Code/Radio`: nine Teensy environments pass; the obsolete Uno environment assumes `Serial1`.
- `Spaceport/Code/Radio-Transceiver`: its legacy Uno/Teensy targets use STM32 pin names, and the STM32L431 custom board lacks a linker script. Newer jhauerst MPM work remains unmerged.
- `Spaceport/Code/STM32FC`: now resolves Astra `main`, but its old sensor adapters use the previous Astra API and reference a removed GPS header.
- `Spaceport/Code/Teensy-Based-Avionics`: the Teensy path references a deliberately removed old Kalman implementation; the experimental STM path lacks a FatFs configuration.

## Before updating `main`

1. Review the candidate's PCB conflict choices, especially the Teensy MCU project metadata and the sanitized flight-computer import.
2. Decide whether CI should build only maintained targets or whether each legacy target should be repaired/archived. The current all-project workflow will fail on `main` for the known reasons above.
3. Coordinate separately with jhauerst on his four preserved branches.
4. Fast-forward `main` to the reviewed candidate, then delete only branches covered by merged history or the archive tags above.

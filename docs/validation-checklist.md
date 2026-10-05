# STM32FC bench and flight readiness checklist

Copy this checklist into a mission-specific test record. Record the board
revision, operator, date, software commits/tags, toolchain versions, equipment,
observations, and artifact locations for every run. A checked box without saved
evidence is not a completed verification.

## 1. Software baseline

- [ ] Review the mission changes and all transitive dependency revisions.
- [ ] Replace moving branch dependencies with reviewed tags or commit hashes.
- [ ] Record the SRAD-Avionics, Astra-Rocket, Astra, and Astra-Support revisions.
- [ ] Run `astra-support doctor` in each relevant checkout.
- [ ] Run the documented Astra and Astra-Rocket build/test matrices.
- [ ] Run `pio run` in `Side_Projects/STM32FC` from a clean checkout.
- [ ] Archive build output, firmware image, map file, configuration, and hashes.

Stop here if a required build or test fails. Do not repair unrelated behavior as
part of release preparation without review and a separate change record.

## 2. Unpowered board review

- [ ] Match the assembled board revision to the reviewed schematic and PCB.
- [ ] Review component orientation, soldering, connectors, and visible damage.
- [ ] Verify power-rail resistance/continuity and absence of shorts using the
      team's approved electrical procedure.
- [ ] Cross-check the source pin table against the actual board and MCU functions.
- [ ] Confirm that recovery outputs and other hazardous loads are physically
      safed or replaced by approved inert test loads.

## 3. Controlled power and boot

- [ ] Use current-limited bench power and the team's approved bring-up procedure.
- [ ] Confirm expected rails, current draw, reset behavior, and clock/USB startup.
- [ ] Capture the USB event log from boot through `AstraRocket initialized`.
- [ ] Confirm that the dual-fast-blink initialization failure indication is
      detectable and that its logged root cause is actionable.
- [ ] Confirm the heartbeat and telemetry LEDs match their documented events.

## 4. Sensors and state inputs

- [ ] Confirm identity and initialization of the BMI088, DPS368, SAM-M10Q,
      MMC5603NJ, H3LIS331DL, and battery monitor.
- [ ] Exercise each physical axis in a known orientation and verify sign, scale,
      units, and the configured mounting transforms.
- [ ] Compare barometric altitude and temperature with a trusted reference.
- [ ] Verify GPS fix, coordinates, altitude, heading, and raw velocity behavior in
      a suitable outdoor test.
- [ ] Compare battery telemetry across the operating range with calibrated
      measurements; review divider and correction values.
- [ ] Save raw logs and acceptance limits, including failed or marginal runs.

## 5. Storage and recovery

- [ ] Confirm eMMC initialization using the source pin assignment.
- [ ] Verify event and data logs are created, nonempty, parseable, and uniquely
      named across repeated boots.
- [ ] Verify expected 10/50/10 Hz logging behavior with timestamps.
- [ ] Exercise `FILE/LS` and `FILE/CP` and compare recovered files byte-for-byte.
- [ ] Test destructive `FILE/RM` and `FILE/CLEAR` only on disposable test data.
- [ ] Power-cycle during a noncritical write test and document filesystem recovery
      behavior before accepting storage for flight data.

## 6. Communications

- [ ] Verify the 115200-baud UART electrical levels, pin direction, and framing.
- [ ] Confirm the hub learns the `ARC_ADDR_FC_N` route from 1 Hz heartbeats.
- [ ] Decode and validate ARC flight telemetry at 10 Hz against local logs.
- [ ] Verify the wrapped data-radio downlink at 1 Hz through the intended hub and
      radio path; do not infer this from LED activity alone.
- [ ] Test loss, corruption, reconnect, and unavailable-ground scenarios without
      blocking flight-software updates.
- [ ] Review and set `ARC_DEBUG_MIRROR` deliberately for the release build.

## 7. Simulation and mission behavior

- [ ] Run nominal and off-nominal SITL/HITL scenarios with saved inputs and logs.
- [ ] Verify stage transitions, timing, reset behavior, and telemetry mapping.
- [ ] Confirm Astra-Rocket recovery-stage detection is treated as estimation only;
      it is not proof of physical deployment behavior.
- [ ] Review all mission thresholds and configuration values with the responsible
      subsystem owners.

## 8. Recovery and hazardous-output tests

- [ ] Obtain the team's required safety approval and establish a controlled test
      area before connecting any energetic or hazardous hardware.
- [ ] Begin with isolated, current-limited, inert loads and independent measurement.
- [ ] Verify safing, arming interlocks, continuity reporting, channel mapping,
      command inhibition, reset behavior, and fault handling against an approved
      test procedure.
- [ ] Conduct any energetic test only under the organization's authorized safety
      procedure and qualified supervision; this checklist is not that procedure.
- [ ] Require independent review of results before integrated or flight use.

## 9. Integrated ground test and release

- [ ] Repeat representative operation with the assembled avionics stack, power
      system, hub, radios, storage, sensors, and approved inert recovery loads.
- [ ] Verify thermal, vibration, duration, and power conditions defined by the
      mission test plan.
- [ ] Recover and review every local and ground-side log; reconcile timestamps and
      missing packets.
- [ ] Resolve or formally accept every anomaly with an owner and rationale.
- [ ] Tag the accepted source baseline and archive reproducible build instructions.
- [ ] Record final firmware hashes and verify the programmed hardware matches them.
- [ ] Freeze configuration after approval; any later change reopens the affected
      verification stages.

## 10. Post-test or post-flight

- [ ] Preserve original media before deleting, clearing, or transforming logs.
- [ ] Recover and hash eMMC, USB, hub, radio, and ground-station artifacts.
- [ ] Record resets, storage errors, sensor-health changes, dropped links, and stage
      discrepancies.
- [ ] File corrective work separately from the preserved mission baseline.

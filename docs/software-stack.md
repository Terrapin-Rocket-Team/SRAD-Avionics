# Flight software stack and integration guide

This is the central user-facing workflow for the current Terrapin Rocket Team
flight-software stack. Each library remains authoritative for its own API.

## Stack ownership

```text
Astra-Support  setup, diagnostics, builds, tests, simulation
      ↓
Astra          sensors, state estimation, logging, HITL/SITL primitives
      ↓
Astra-Rocket   rocket stages, flight defaults, ARC command bridge
      ↓
SRAD-Avionics  board wiring, firmware, radios, storage, mission integration
```

| Repository | Use it for | Authoritative user docs |
| --- | --- | --- |
| Astra-Support | Installing the CLI; checking a checkout; running managed builds/tests/simulation | [README](https://github.com/Terrapin-Rocket-Team/Astra-Support#readme) |
| Astra | Sensor, state, filter, logging, and HITL/SITL APIs | [Astra documentation](https://terrapin-rocket-team.github.io/Astra/) |
| Astra-Rocket | Rocket configuration, flight stages, logging rates, and ARC bridge | [Astra-Rocket documentation](https://terrapin-rocket-team.github.io/Astra-Rocket/) |
| SRAD-Avionics | Hardware-specific source, pin mappings, telemetry, storage, and complete firmware | This guide and project-local READMEs |

Do not copy an upstream API description into this repository. Link to its
owning documentation and document only the integration decision made here.

## Fresh-machine setup

Install Git, Python 3.10 or newer, `pipx`, and a native C++ compiler. On Ubuntu
24.04 or WSL:

```bash
sudo apt update
sudo apt install git pipx g++
pipx ensurepath
```

Open a new shell, then install the shared CLI:

```bash
pipx install "git+https://github.com/Terrapin-Rocket-Team/Astra-Support.git@main"
astra-support --version
```

Clone Avionics with its submodule:

```bash
git clone --recurse-submodules https://github.com/Terrapin-Rocket-Team/SRAD-Avionics.git
cd SRAD-Avionics
```

For an existing clone:

```bash
git submodule update --init --recursive
```

## Current integration check

`Side_Projects/STM32FC` is the current compile smoke test for the published
Astra-Rocket → Astra dependency chain:

```bash
cd Side_Projects/STM32FC
pio run
```

This project currently selects dependencies from the canonical `main`
branches. For a flight release, replace moving branch references with reviewed
tags or commit hashes and record that version set with the mission.

## Starting application code

The maintained STM32 integration follows this lifetime pattern:

```cpp
using namespace astra;
using namespace astra_rocket;

AstraRocketConfig config;
AstraRocket rocket(config);

void setup() {
    config.withFlightLogRate(50);
    config.with6DoFIMU(&imu)
          .withBaro(&baro)
          .withGPS(&gps)
          .withDataLogs(dataLogSinks, 1)
          .withEventLogs(eventLogSinks, 2);

    if (!rocket.init()) {
        // Stop in a hardware-visible failure state.
    }
}

void loop() {
    rocket.update();
}
```

The config and rocket must outlive `setup()`. Set sensor mounting transforms,
bus pins, logging sinks, and transport objects before calling `init()`.

## What a successful build proves

A successful `pio run` proves that the selected source and dependency versions
compile for the target. It does not validate:

- board power, clocks, pin mappings, or buses;
- sensor identity, axes, calibration, or update rates;
- EMMC/SD writes and post-flight file recovery;
- radio or ARC interoperability;
- HITL timing and stage transitions;
- actuator, continuity, pyro, or recovery behavior; or
- PCB design-rule correctness or fabrication readiness.

Record those checks in a mission-specific procedure. Never infer deployment
success from Astra-Rocket's recovery-stage estimates.

For the current STM32H723 flight-computer implementation, see the
[source-level STM32FC integration reference](stm32fc-integration.md). Copy the
[bench and flight readiness checklist](validation-checklist.md) into the mission
test record and retain the resulting evidence.

## Known-good software handoff check

The 2026 clean-room pass established that:

- Astra-Support installed into a new Ubuntu environment;
- Astra's managed matrix built and its native tests passed;
- Astra-Rocket's managed matrix built and its native tests passed; and
- `Side_Projects/STM32FC` compiled against the published library commits.

See [the dated consolidation record](2026-consolidation-status.md) for exact
commits, test counts, known legacy failures, and limitations. Re-run the checks
after dependency or toolchain changes.

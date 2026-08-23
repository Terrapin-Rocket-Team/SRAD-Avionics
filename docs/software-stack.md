# Flight software: install, test, simulate, and integrate

This is the canonical user workflow for the Terrapin Rocket Team flight-software
stack. Start here on a new machine. Each repository remains authoritative for
its own API and hardware details; this page owns the order in which the pieces
are installed and verified together.

The commands below were exercised during the August 2026 handoff pass in a
fresh Ubuntu 24.04 WSL distro. Software-only validation cannot certify attached
sensors, storage, radios, actuators, recovery systems, or flight readiness.

## 1. Understand the stack

```text
Astra-Support  installs tools; diagnoses; builds; tests; runs SITL/HITL
      |
      v
Astra          sensors; state estimation; logging; HITL/SITL primitives
      |
      v
Astra-Rocket   rocket stages; flight defaults; ARC command bridge
      |
      +-------------------------+
      v                         v
SRAD-Avionics                 Airbrake
board/mission integration     controller, motor, telemetry, custom simulation
```

| Repository | Current user entry point |
| --- | --- |
| [Astra-Support](https://github.com/Terrapin-Rocket-Team/Astra-Support) | CLI installation and command reference |
| [Astra](https://github.com/Terrapin-Rocket-Team/Astra) | [Astra user documentation](https://terrapin-rocket-team.github.io/Astra/) |
| [Astra-Rocket](https://github.com/Terrapin-Rocket-Team/Astra-Rocket) | [Astra-Rocket user documentation](https://terrapin-rocket-team.github.io/Astra-Rocket/) |
| [Airbrake](https://github.com/Terrapin-Rocket-Team/Airbrake) | Root README, then `Astra-Rocket/` |
| [SRAD-Avionics](https://github.com/Terrapin-Rocket-Team/SRAD-Avionics) | This page, `Side_Projects/STM32FC`, and the validation checklist |

Use Astra for reusable flight-software behavior, Astra-Rocket for reusable
rocket behavior, and the application repositories for wiring and mission
decisions. Do not fix an application problem by copying an upstream library
into it.

## 2. Prepare a clean development environment

WSL or native Linux is recommended for reproducible command-line work. Keep
repositories in the Linux filesystem (for example `~/trt`), not under
`/mnt/c`, because PlatformIO performs many small file operations.

On Windows, install Ubuntu 24.04 from an elevated PowerShell terminal if it is
not already available:

```powershell
wsl --install -d Ubuntu-24.04
```

Inside Ubuntu:

```bash
sudo apt update
sudo apt install git pipx g++
pipx ensurepath
```

Close and reopen the shell. Install PlatformIO for direct `pio` commands and
install the shared Astra CLI:

```bash
pipx install platformio
pipx install "git+https://github.com/Terrapin-Rocket-Team/Astra-Support.git@main"
pio --version
astra-support --version
```

For an existing Astra-Support installation, refresh it explicitly:

```bash
pipx install --force "git+https://github.com/Terrapin-Rocket-Team/Astra-Support.git@main"
```

Do not use `sudo pip`, install PlatformIO into the system Python, or manually
inject simulation packages. Astra-Support declares its simulation dependencies.

## 3. Clone the current repositories

```bash
mkdir -p ~/trt
cd ~/trt
git clone https://github.com/Terrapin-Rocket-Team/Astra-Support.git
git clone https://github.com/Terrapin-Rocket-Team/Astra.git
git clone https://github.com/Terrapin-Rocket-Team/Astra-Rocket.git
git clone https://github.com/Terrapin-Rocket-Team/Airbrake.git
git clone --recurse-submodules https://github.com/Terrapin-Rocket-Team/SRAD-Avionics.git
```

For an existing Avionics checkout:

```bash
git -C ~/trt/SRAD-Avionics submodule update --init --recursive
```

Before changing anything, record the baseline:

```bash
for repo in Astra-Support Astra Astra-Rocket Airbrake SRAD-Avionics; do
  git -C "$HOME/trt/$repo" status --short --branch
  git -C "$HOME/trt/$repo" rev-parse HEAD
done
```

Expected handoff state is a clean `main` checkout. Stop and preserve unexpected
local modifications before pulling, switching branches, or cleaning builds.

## 4. Verify the libraries and applications

Run diagnostics first. `doctor` checks the selected project, PlatformIO,
compiler, datasets, and custom simulation hooks:

```bash
astra-support doctor --project ~/trt/Astra
astra-support doctor --project ~/trt/Astra-Rocket
astra-support doctor --project ~/trt/Airbrake/Astra-Rocket
```

Then run the managed matrices:

```bash
astra-support test --project ~/trt/Astra --clean --no-progress
astra-support test --project ~/trt/Astra-Rocket --clean --no-progress
astra-support test --project ~/trt/Airbrake/Astra-Rocket --clean --no-progress
```

Four parallel workers are used by default. On a machine with enough memory:

```bash
astra-support test --project ~/trt/Astra --jobs 8 --no-progress
```

`--clean` removes build outputs but does not update dependencies. Use
`--update-deps` only when intentionally evaluating dependency changes. Initial
clean runs compile each test program and are slower; normal repeat runs reuse
the build cache while retaining parallel suites.

Finally compile the maintained Avionics integration target:

```bash
cd ~/trt/SRAD-Avionics/Side_Projects/STM32FC
pio run
```

Do not begin with a repository-wide SRAD-Avionics build. It deliberately keeps
historical projects that are known not to compile. The STM32FC build proves the
current Astra-Rocket -> Astra application chain compiles; it does not prove the
board works.

## 5. Use Astra and Astra-Rocket in an application

Application `platformio.ini` files should depend on Astra-Rocket `main`, which
in turn owns the Astra dependency:

```ini
lib_deps =
  https://github.com/Terrapin-Rocket-Team/Astra-Rocket.git#main
```

During active development, moving `main` references make integration testing
easy. Before a mission freeze, replace them with reviewed tags or full commit
hashes and record the complete version set.

The application config and `AstraRocket` instance must outlive `setup()`:

```cpp
using namespace astra;
using namespace astra_rocket;

AstraRocketConfig config;
AstraRocket rocket(config);

void setup() {
    config.with6DoFIMU(&imu)
          .withBaro(&baro)
          .withGPS(&gps);

    if (rocket.init() < 0) {
        // Enter a hardware-visible safe failure state.
    }
}

void loop() {
    rocket.update();
}
```

Use the Astra documentation for sensors, transforms, state, and logging. Use
the Astra-Rocket documentation for stages, configuration, and ARC behavior.
Use the application repository for actual pin assignments and mission values.

## 6. Run software-in-the-loop (SITL)

SITL executes native flight software and sends simulated sensor packets over a
local TCP connection. It is the first full behavior check after builds/tests.

### Generic Astra SITL

```bash
cd ~/trt/Astra
astra-support sim list --project .
astra-support sim run --project . --mode sitl --source physics --no-plot
```

The native executable is built automatically if missing. Add `--build` after
changing source or `platformio.ini`.

### Airbrake closed-loop SITL

```bash
cd ~/trt/Airbrake/Astra-Rocket
astra-support sim list --project .
astra-support sim run \
  --project . \
  --mode sitl \
  --source airbrake \
  --target-apogee 8382 \
  --no-plot
```

The target apogee is in metres. The Airbrake hook feeds measured flap angle
back into its propagator, allowing motor/controller behavior to affect the
simulated trajectory. A normal run proceeds from pad through landing and writes
`sim_log_*.csv`. Preserve the command, source data, console output, SITL process
log, and CSV when the result is used as test evidence.

`Side_Projects/STM32FC` currently has no native environment or project-specific
simulation hook. Its maintained handoff gate is compilation plus hardware bench
verification. Do not claim Avionics SITL coverage based on the library or
Airbrake runs.

## 7. Run hardware-in-the-loop (HITL)

HITL replaces sensor inputs with simulated packets sent to physical hardware.
It does not make connected actuators or outputs safe.

Before connecting the runner:

1. Record the source revisions and build environment.
2. Disconnect energetic devices and physically safe hazardous outputs.
3. Use approved inert loads and current-limited bench power.
4. Confirm the correct serial port and voltage levels.
5. Ensure the firmware was built specifically for HITL rather than flight.

### Generic Astra HITL

Build an application that calls `withHITL()` and selects the serial interface,
as documented in the [Astra HITL guide](https://terrapin-rocket-team.github.io/Astra/user-guide/hitl/).
After flashing it:

```bash
cd ~/trt/Astra
astra-support sim run \
  --project . \
  --mode hitl \
  --port /dev/ttyACM0 \
  --baud 115200 \
  --source physics \
  --real-time \
  --no-plot
```

### Airbrake HITL

Airbrake supplies a dedicated `teensy41_hitl` environment. It selects Astra's
HITL sensors and must never be used as flight firmware:

```bash
cd ~/trt/Airbrake/Astra-Rocket
pio run -e teensy41_hitl
# Upload with the team's approved Teensy/PlatformIO procedure.

astra-support sim run \
  --project . \
  --mode hitl \
  --port /dev/ttyACM0 \
  --baud 115200 \
  --source airbrake \
  --target-apogee 8382 \
  --real-time \
  --no-plot
```

On Windows, use the correct `COM` port. Airbrake HITL may exercise the physical
motor path; keep the mechanism restrained only according to the approved test
fixture/procedure and maintain an independent means of removing power.

The current SRAD STM32FC application does not select HITL sensors and therefore
must not be connected to this runner with an expectation of Avionics HITL.
Adding a dedicated Avionics HITL environment is future development requiring
hardware-owner review.

## 8. Move from simulation to Avionics bench integration

After software tests and applicable simulation pass:

1. Read the [STM32FC integration reference](stm32fc-integration.md).
2. Copy the [validation checklist](validation-checklist.md) into a dated,
   mission-specific test record.
3. Verify board revision, source pin mappings, sensor identities and axes,
   storage, ARC routing, radio framing, and power behavior independently.
4. Compare local logs, downlinked telemetry, and simulated expectations.
5. Record every anomaly; do not silently tune flight thresholds during a
   release test.

A successful compile, native unit test, SITL run, or HITL run proves only its
own layer. None is a substitute for electrical review, controlled board
bring-up, integrated ground testing, or the team's flight-safety process.

## 9. Freeze and hand off a mission baseline

Before declaring a configuration ready for another team:

- make every relevant working tree clean;
- record all five repository commits and tool versions;
- pin moving dependencies to reviewed revisions;
- archive build output, firmware binaries, maps, hashes, simulation inputs,
  console output, and generated logs;
- complete and review the mission validation checklist;
- tag the accepted source baseline; and
- document unresolved warnings, skipped hardware tests, and responsible owners.

The dated [2026 consolidation status](2026-consolidation-status.md) describes
the repository cleanup baseline and known legacy limitations. Treat it as a
historical handoff record; this page remains the current operational sequence.

## Troubleshooting quick reference

- `astra-support` not found: open a new shell after `pipx ensurepath`.
- `pio` not found: install PlatformIO separately with `pipx install platformio`.
- Unknown environment: run `pio project config` in the directory containing the
  intended `platformio.ini`.
- Stale source/configuration in SITL: add `--build`.
- Dependency changes not appearing: run the relevant matrix once with
  `--update-deps`.
- Serial port busy: close PlatformIO monitor and other terminal programs before
  HITL.
- Airbrake atmosphere import failure: refresh Astra-Support from `main`; do not
  manually edit the installed environment.
- Hardware behavior differs from simulation: stop and treat it as a failed
  integration test, not as permission to adjust values without review.

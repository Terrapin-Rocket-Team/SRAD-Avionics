# SRAD-Avionics

Code, KiCad projects, and supporting experiments for the Terrapin Rocket Team
avionics subteam. The canonical branch is `main`.

This is a historical engineering repository as well as a source repository.
Some PlatformIO projects are maintained integration targets; others are
preserved prototypes and are known not to build. Do not assume that every
directory represents current flight hardware.

## Repository layout

- `Spaceport/` contains competition-rocket firmware, PCBs, and mechanical work.
- `Side_Projects/` contains flight-computer integrations, radio work, training,
  and experiments.
- `Apollo/` contains the Apollo project material organized by term.
- `docs/2026-consolidation-status.md` records the dated branch consolidation,
  verified builds, and known legacy failures.

KiCad boards normally consist of `.kicad_pro`, `.kicad_sch`, and `.kicad_pcb`
files. Open the `.kicad_pro` file to work on a board.

## Clone and prerequisites

Install Git and PlatformIO Core, or VS Code with the PlatformIO extension. Clone
the repository and its Ground Station submodule with:

```bash
git clone --recurse-submodules https://github.com/Terrapin-Rocket-Team/SRAD-Avionics.git
cd SRAD-Avionics
```

For an existing clone:

```bash
git submodule update --init --recursive
```

Open the specific directory containing the desired `platformio.ini` in VS Code,
or run PlatformIO from that directory. Do not run a repository-wide build as a
first setup check: the repository intentionally retains known-broken legacy
projects.

## Current integration smoke test

`Side_Projects/STM32FC` is the maintained smoke test for the current
Astra-Rocket -> Astra dependency chain:

```bash
cd Side_Projects/STM32FC
pio run
```

This compiles firmware only. Uploading, hardware-in-the-loop testing, deployment
outputs, radios, and sensors require the matching hardware and must be verified
separately before flight.

## Build status and legacy projects

See [the 2026 consolidation status](docs/2026-consolidation-status.md) for the
last clean build matrix. At that handoff point, ten projects passed and nine
legacy or mixed projects had documented failures. The existing GitHub Actions
workflow still attempts every `platformio.ini`, but its shell command does not
reliably propagate individual project failures. A green PlatformIO workflow is
therefore not a substitute for the documented matrix or the STM32 smoke test.

The repository-wide KiCad DRC workflow is known to fail across both the
pre-consolidation and consolidated histories. Review each board's DRC output
before fabrication.

Owner-controlled jhauerst branches were deliberately excluded from the 2026
consolidation and should be coordinated with their owner rather than merged or
deleted as repository cleanup.

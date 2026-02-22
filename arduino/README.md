# Arduino Water Automation Logic

This sketch is intentionally split into multiple modules (`src/*.h|cpp`) so logic, sensor IO, motor control, and command processing remain maintainable.

## Features implemented

- Two motors: borewell and sump, with borewell priority in auto/override mode.
- Two tanks:
  - Overhead tank levels: below_critical, critical, low, med, high.
  - Sump tank levels: below_critical, critical, low, high.
- Logic:
  - In `auto`, when overhead is `low/critical/below_critical`, fill until `high`.
  - Motor priority: try borewell first; fallback to sump.
  - `override_fill` ignores overhead-level trigger and always fills to `high`.
  - `manual` mode allows full motor choice and ignores tank-level safety limits.
- Safety:
  - Dry-run detection for both motors stops motor immediately.
  - Sump `critical` emits warning.
  - Sump `below_critical` stops sump motor unless in manual mode.
- Serial command interface for mode/motor/force-start controls.
- Periodic serial status with levels and motor state.

## Serial commands

- `mode auto`
- `mode override`
- `mode manual`
- `motor borewell`
- `motor sump`
- `motor none`
- `force on`
- `force off`
- `help`

## Pin mapping

Edit `src/Config.h` to match hardware.

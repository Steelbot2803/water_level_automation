# Arduino Water Level Automation Logic

This folder contains a multi-file Arduino implementation for:
- 2 motors: `borewell` (priority) and `sump transfer`
- 2 tanks: overhead tank (4 levels) and sump tank (3 levels)
- safety controls including dry-run lockout and sump-below-critical stop
- commands for automatic, override fill, and manual motor selection
- status output over serial

## Files
- `WaterAutomation.ino` - setup/loop wiring
- `config.h` - pin map and timing constants
- `controller.h/.cpp` - state model and control logic
- `io.h/.cpp` - serial command parser
- `status.h/.cpp` - periodic telemetry output

## Serial commands
- `auto`
- `manual`
- `override`
- `motor borewell`
- `motor sump`
- `motor stop`
- `status`
- `help`

## Key behavior
- In **auto mode**:
  - If overhead is `low/critical/empty`, start filling.
  - Borewell motor is tried first; if unavailable/dry-run locked, fallback to sump transfer.
  - Stop when overhead reaches `high`.
- **Override** acts like auto fill-to-high regardless of current overhead level.
- In **manual mode**:
  - Overhead/sump level demand is ignored.
  - Dry-run and sump-below-critical safety remain active.
- If sump reaches `critical`, warning is emitted.
- If sump goes `below critical`, motor is stopped unless manual mode is active.

Adjust pin polarity/threshold wiring in `config.h` according to your sensor type.

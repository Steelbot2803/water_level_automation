# Arduino Water Level Automation Logic

This folder contains a multi-file Arduino implementation for:
- 2 motors: `borewell` (priority) and `sump transfer`
- 2 tanks: overhead tank (critical/low/medium/high) and sump tank (critical/low/high)
- safety controls including dry-run lockout and sump-critical stop
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
  - If overhead is `low/critical`, start filling.
  - Borewell motor is tried first; if unavailable/dry-run locked, fallback to sump transfer.
  - Stop when overhead reaches `high`.
- **Override** acts like auto fill-to-high regardless of current overhead level.
- In **manual mode**:
  - Overhead/sump level demand is ignored.
  - Dry-run and sump-critical safety remain active.
- `critical` is treated as the point where the LOW-level sensor is no longer active for both tanks.
- In auto mode, if sump is `critical`, motor operation is blocked for pump protection.

Adjust pin polarity/threshold wiring in `config.h` according to your sensor type.


## HiveMQ integration
- The sketch now publishes status to MQTT and accepts the same command strings over MQTT as serial.
- Create `arduino/WaterAutomation/secrets.h` from `secrets.example.h`.
- Add required libraries in Arduino IDE/CLI:
  - `WiFi` (or replace with your board-specific WiFi library)
  - `PubSubClient`
- MQTT topics:
  - Command subscribe: `water-system/cmd`
  - Status publish: `water-system/status`

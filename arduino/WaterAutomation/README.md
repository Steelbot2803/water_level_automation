# Arduino Water Level Automation Logic

This folder contains a multi-file Arduino implementation for:

- 2 motors: `borewell` (priority) and `sump`
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
  - Borewell motor is tried first; if unavailable/dry-run locked, fallback to sump.
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
- MQTT/WiFi connection attempts are non-blocking so serial status output is not held up when network/cloud is unavailable.
- Serial status now includes `wifi_connected` and `mqtt_connected` flags for quick diagnostics.
- Create `arduino/WaterAutomation/secrets.h` from `secrets.example.h`.
- Add required libraries in Arduino IDE/CLI:
  - `WiFi` (or replace with your board-specific WiFi library)
  - `PubSubClient`
- MQTT topics:
  - Command subscribe: `water-system/cmd`
  - Status publish: `water-system/status`
- Status payload shape:

```json
{
  "mode": "auto",
  "override": false,
  "manual_target": "none",
  "overhead": "low",
  "sump": "high",
  "motor": "borewell",
  "borewell_status": "running",
  "sump_status": "stopped",
  "sump_warning": false
}
```

- Field notes:
  - `manual_target` reports the requested manual motor only while the controller is in manual mode.
  - `borewell_status` and `sump_status` mirror the controller runtime states: `stopped`, `starting`, `running`, `dry_run_lock`, `blocked_by_safety`.
  - `sump_warning` goes true when the sump reaches critical or the critical-warning latch has been set.

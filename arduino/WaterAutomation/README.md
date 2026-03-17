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

- `auto`           — automatic mode; also clears emergency stop
- `manual`         — manual mode; also clears emergency stop
- `override`       — fill to HIGH regardless of overhead level; also clears emergency stop
- `motor borewell` — manual mode: run borewell motor
- `motor sump`     — manual mode: run sump transfer motor
- `motor stop`     — stop active motor (any mode); does not change mode
- `borewell`       — auto mode: prefer borewell motor (default)
- `sump`           — auto mode: prefer sump transfer motor over borewell
- `estop`          — emergency stop all motors immediately (any mode)
- `resume`         — clear emergency stop flag
- `unlock borewell`— clear borewell dry-run latch so it can start again
- `unlock sump`    — clear sump dry-run latch so it can start again
- `status`         — print current status immediately
- `help`           — show command list

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
  "sump_warning": false,
  "emergency_stop": false,
  "auto_prefer_sump": false
}
```

- Field notes:
  - `manual_target` reports the requested manual motor only while the controller is in manual mode.
  - `borewell_status` and `sump_status` mirror the controller runtime states:
    `stopped`, `starting`, `running`, `dry_run_lock`, `sump_critical`.
    - `dry_run_lock` — motor stopped because no flow was detected; requires manual
      `unlock borewell` or `unlock sump` command to clear.
    - `sump_critical` — sump transfer motor specifically stopped because the sump
      tank is at critical level (pump protection).
  - `sump_warning` goes `true` when the sump reaches critical or the
    critical-warning latch has been set.
  - `emergency_stop` goes `true` when an `estop` command has been received and
    remains `true` until `auto`, `manual`, or `resume` is sent.
  - `auto_prefer_sump` goes `true` when the `sump` preference command has been
    sent; `false` (borewell preferred) is the default.

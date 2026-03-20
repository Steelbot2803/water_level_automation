# Water Level Automation

[![Netlify](https://img.shields.io/badge/Deploy-Netlify-00C7B7?logo=netlify&logoColor=white)](https://www.netlify.com/)
[![Svelte](https://img.shields.io/badge/Frontend-Svelte-FF3E00?logo=svelte&logoColor=white)](https://svelte.dev/)
[![HiveMQ](https://img.shields.io/badge/Protocol-HiveMQ-660066?logo=eclipsemosquitto&logoColor=white)](https://www.hivemq.com/)
[![OpenAI Codex](https://img.shields.io/badge/Assisted%20by-Codex-412991)](https://chatgpt.com/codex)
[![Claude](https://img.shields.io/badge/Assisted%20by-Claude-D97757)](https://www.anthropic.com/claude)

Water Level Automation is a two-part system for monitoring and controlling a tank filling setup:

- An Arduino controller that reads tank levels, runs pump logic, enforces safety rules, and publishes telemetry over MQTT
- A mobile-first Svelte PWA that connects over MQTT WebSockets to display status and send commands

The repository is structured so the firmware and web app can be used together or adapted independently.

## AI Attribution

This repository includes substantial AI-assisted development.

- Claude contributed significantly to early system design, control-flow iteration, and end-to-end integration of the firmware and PWA.
- Codex contributed significantly to implementation refinement, UI behavior fixes, documentation, and iterative code changes in the current version.

## Repository Layout

- `arduino/WaterAutomation` - Arduino firmware, controller logic, serial/MQTT command handling, and telemetry output
- `pwa` - SvelteKit progressive web app for live status, manual control, and system actions

## Features

- Automatic overhead tank filling based on sensor levels
- Two-motor support:
  - Borewell motor
  - Sump transfer motor
- Safety controls:
  - Emergency stop
  - Dry-run lockout
  - Sump critical protection
- MQTT status publishing and command subscription
- Browser-based control surface over secure WebSockets
- Manual mode, override mode, and pump preference selection

## System Architecture

1. The Arduino reads tank level inputs and runs the control logic.
2. The Arduino publishes status to the MQTT status topic.
3. The PWA subscribes to the same status topic and renders the live state.
4. The PWA publishes command payloads to the MQTT command topic.
5. The Arduino receives those commands and routes them through the same command handler used for serial input.

Default topics:

- Command topic: `water-system/cmd`
- Status topic: `water-system/status`

## Arduino Firmware

The Arduino implementation includes:

- `WaterAutomation.ino` - setup and loop wiring
- `config.h` - pin map and timing constants
- `controller.h/.cpp` - state model and control logic
- `io.h/.cpp` - serial command parser
- `status.h/.cpp` - periodic telemetry output

### Arduino Behavior

- In `auto` mode:
  - If the overhead tank is `low` or `critical`, the controller starts filling.
  - Borewell is preferred by default.
  - If borewell is unavailable or dry-run locked, the controller can fall back to sump transfer.
  - Filling stops when the overhead tank reaches `high`.
- `override` fills the overhead tank to `high` regardless of the current overhead level.
- In `manual` mode:
  - Automatic demand logic is ignored.
  - Safety rules still apply.
- If the sump reaches `critical`, pump operation is blocked where required for protection.

Adjust pin polarity and sensor wiring thresholds in `arduino/WaterAutomation/config.h` for your hardware.

### Arduino Setup

1. Copy `arduino/WaterAutomation/secrets.example.h` to `arduino/WaterAutomation/secrets.h`.
2. Fill in WiFi and MQTT broker credentials.
3. Install the required Arduino libraries for your board and network stack.

Expected MQTT-related dependencies:

- `PubSubClient`
- A compatible WiFi library for your board

## PWA

The PWA is a browser-based MQTT control surface built with SvelteKit.

### PWA Features

- Connects to an MQTT broker over WebSockets
- Subscribes to device telemetry
- Sends the same literal command strings accepted by the firmware
- Displays motor runtime states, connection status, alarms, and control actions

### PWA Environment Variables

Copy `pwa/.env.example` to `pwa/.env` and set:

```bash
PUBLIC_MQTT_HOST=
PUBLIC_MQTT_PORT=8884
PUBLIC_MQTT_PATH=/mqtt
PUBLIC_MQTT_USE_SSL=true
PUBLIC_MQTT_USERNAME=
PUBLIC_MQTT_PASSWORD=
PUBLIC_MQTT_COMMAND_TOPIC=water-system/cmd
PUBLIC_MQTT_STATUS_TOPIC=water-system/status
PUBLIC_MQTT_CLIENT_ID_PREFIX=water-pwa
```

Notes:

- `8884` is the secure WebSocket port commonly used by HiveMQ Cloud for browser clients.
- `8883` is typically the raw MQTT/TLS port for device clients, not the browser app.
- Restart the dev server after changing `.env`.

### PWA Development

```bash
cd pwa
npm install
npm run dev
```

### PWA Validation

```bash
cd pwa
npm run check
npm run build
```

## Commands

The firmware accepts these command strings over both serial and MQTT.

### Mode Commands

| Payload    | Effect                                                                           |
| ---------- | -------------------------------------------------------------------------------- |
| `auto`     | Automatic mode. Also clears emergency stop.                                      |
| `manual`   | Manual mode. Also clears emergency stop.                                         |
| `override` | Fill overhead tank to `high` regardless of current level. Clears emergency stop. |

### Manual Motor Commands

| Payload          | Effect                                                 |
| ---------------- | ------------------------------------------------------ |
| `motor borewell` | Switch to manual mode and run the borewell motor.      |
| `motor sump`     | Switch to manual mode and run the sump transfer motor. |
| `motor stop`     | Stop the active motor without changing mode.           |

### Auto Preference Commands

| Payload    | Effect                                                  |
| ---------- | ------------------------------------------------------- |
| `borewell` | In auto mode, prefer borewell motor.                    |
| `sump`     | In auto mode, prefer sump transfer motor over borewell. |

### Safety Commands

| Payload           | Effect                                                       |
| ----------------- | ------------------------------------------------------------ |
| `estop`           | Emergency stop all motors immediately in any mode.           |
| `resume`          | Clear the emergency stop flag.                               |
| `unlock borewell` | Clear the borewell dry-run latch so it can start again.      |
| `unlock sump`     | Clear the sump transfer dry-run latch so it can start again. |

### Diagnostic Commands

| Payload  | Effect                                            |
| -------- | ------------------------------------------------- |
| `status` | Force the controller to publish a status payload. |
| `help`   | Print the available command list over serial.     |

## Status Payload

Typical status payload:

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

### Field Notes

- `manual_target` reports the requested manual motor while the controller is in manual mode.
- `borewell_status` and `sump_status` can be:
  - `stopped`
  - `starting`
  - `running`
  - `dry_run_lock`
  - `sump_critical`
- `dry_run_lock` means the motor stopped because no flow was detected and must be manually unlocked.
- `sump_critical` means the sump transfer motor was blocked or stopped for protection.
- `sump_warning` becomes `true` when the sump reaches critical or its warning latch is set.
- `emergency_stop` remains `true` until cleared by `auto`, `manual`, or `resume`.
- `auto_prefer_sump` reflects whether sump has been selected as the preferred auto-fill source.

## Getting Started

To run the full system:

1. Flash the Arduino firmware with the correct sensor, motor, WiFi, and MQTT settings.
2. Confirm the controller can publish to `water-system/status` and subscribe to `water-system/cmd`.
3. Configure the PWA environment to use the same broker and topics.
4. Start the PWA and verify that live status updates and commands flow end to end.

## License

This repository is licensed under the terms in [LICENSE](LICENSE).

For contributor expectations and community standards, see [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).

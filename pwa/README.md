# Water Automation PWA

This app is a browser-based MQTT control surface for the Arduino water automation controller.

## What it does

- Connects to HiveMQ over WebSockets from the browser
- Subscribes to the Arduino status topic
- Publishes the exact command strings accepted by `applyCommand()`
- Displays the widened Arduino status payload, including motor runtime states

## Environment variables

Copy `.env.example` to `.env`. The app auto-connects from these values on page load.

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

- For HiveMQ Cloud in the browser, use the secure WebSocket port `8884`.
- Port `8883` is the raw MQTT/TLS port used by the Arduino, not the PWA.
- If you change `.env`, restart the dev server so Vite reloads the public env values.

## Development

```bash
npm install
npm run dev
```

## Validation

```bash
npm run check
npm run build
```

## Command mapping

The PWA publishes these literal string payloads to the command topic.

### Mode commands

| Payload    | Effect                                                                          |
| ---------- | ------------------------------------------------------------------------------- |
| `auto`     | Automatic mode. Also clears emergency stop.                                     |
| `manual`   | Manual mode. Also clears emergency stop.                                        |
| `override` | Fill overhead tank to HIGH, regardless of current level. Clears emergency stop. |

### Motor commands (manual mode)

| Payload          | Effect                                                 |
| ---------------- | ------------------------------------------------------ |
| `motor borewell` | Switch to manual mode and run the borewell motor.      |
| `motor sump`     | Switch to manual mode and run the sump transfer motor. |
| `motor stop`     | Stop the active motor. Does **not** change mode.       |

### Auto-mode preference commands

| Payload    | Effect                                                  |
| ---------- | ------------------------------------------------------- |
| `borewell` | In auto mode, prefer borewell motor (default).          |
| `sump`     | In auto mode, prefer sump transfer motor over borewell. |

### Safety commands

| Payload           | Effect                                                              |
| ----------------- | ------------------------------------------------------------------- |
| `estop`           | Emergency stop — halt all motors immediately in any mode.           |
| `resume`          | Clear the emergency stop flag (leaves mode unchanged).              |
| `unlock borewell` | Clear the borewell dry-run latch so the motor can start again.      |
| `unlock sump`     | Clear the sump transfer dry-run latch so the motor can start again. |

### Diagnostic commands

| Payload  | Effect                                                     |
| -------- | ---------------------------------------------------------- |
| `status` | Force the Arduino to publish a status payload immediately. |

> **Note:** All of these commands are also accepted over the serial port.
> The PWA sends them by publishing to `water-system/cmd`.
> The Arduino receives them via its MQTT subscription and runs them through
> the same `applyCommand()` function that handles serial input.

## Expected status payload

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
 "estop": false
}
```

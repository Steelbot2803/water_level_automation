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

The PWA publishes these literal MQTT payloads:

- `auto`
- `manual`
- `override`
- `motor borewell`
- `motor sump`
- `motor stop`
- `status`

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
  "sump_transfer_status": "stopped",
  "sump_warning": false
}
```

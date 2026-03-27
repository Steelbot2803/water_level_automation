# Water Level Automation

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Frontend: SvelteKit](https://img.shields.io/badge/Frontend-SvelteKit-FF3E00?logo=svelte&logoColor=white)](https://svelte.dev/)
[![Broker: MQTT](https://img.shields.io/badge/Protocol-MQTT-660066)](https://mqtt.org/)

Water Level Automation is an open source Arduino + PWA project for tank monitoring and pump control.

It provides:
- Firmware for sensor reading, safety checks, and pump automation.
- A mobile-friendly web app for live telemetry and remote commands over MQTT/WebSockets.

## Project Status

This project is actively maintained. Issues and pull requests are welcome.

## Repository Layout

- `arduino/WaterAutomation/` — firmware, automation logic, serial command parser, MQTT integration.
- `pwa/` — SvelteKit progressive web app.
- `pcb/` — board and fabrication assets.

## Core Features

- Automatic overhead-tank filling (`auto` mode).
- Manual motor control (`manual` mode).
- Override fill-to-high mode (`override`).
- Dry-run lock detection and manual unlock commands.
- Sump critical protection and warning state.
- Emergency stop / resume controls.
- MQTT status publishing and command subscription.

## Safety Notes

This software controls motors and water systems. Validate all wiring, relay ratings, and fail-safe behavior before real-world use.

Recommended precautions:
- Add hardware interlocks and overload protection.
- Test dry-run, emergency stop, and power-loss recovery.
- Never run unattended until your safety tests pass.

## Firmware Setup (Arduino)

1. Copy `arduino/WaterAutomation/secrets.example.h` to `arduino/WaterAutomation/secrets.h`.
2. Set Wi-Fi and MQTT credentials in `secrets.h`.
3. Open `arduino/WaterAutomation/WaterAutomation.ino` in Arduino IDE.
4. Install board support and required libraries (`PubSubClient`, `WiFiS3`, board-specific dependencies).
5. Build and flash the board.

### Default Topics

- Command topic: `water-system/cmd`
- Status topic: `water-system/status`

### Supported Commands

| Command | Description |
|---|---|
| `auto` | Enable automatic mode and clear e-stop. |
| `manual` | Enable manual mode and clear e-stop. |
| `override` | Fill until overhead tank reaches `high`. |
| `motor borewell` | Manual: run borewell motor. |
| `motor sump` | Manual: run sump motor. |
| `motor stop` | Stop active motor. |
| `borewell` | Auto preference: borewell. |
| `sump` | Auto preference: sump. |
| `estop` | Emergency stop motors. |
| `resume` | Clear emergency stop. |
| `unlock borewell` | Clear borewell dry-run lock. |
| `unlock sump` | Clear sump dry-run lock. |
| `reset` | Reboot controller. |
| `reset state` | Reset persisted state defaults. |
| `status` | Force serial status print. |
| `help` | Print command list on serial. |

## PWA Setup

1. Copy `pwa/.env.example` to `pwa/.env`.
2. Set MQTT WebSocket values.
3. Install dependencies and run locally:

```bash
cd pwa
npm install
npm run dev
```

### PWA Environment Variables

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

## Development Checks

```bash
cd pwa
npm run lint
npm run check
npm run build
```

## Contributing

Contributions are welcome.

1. Fork the repo.
2. Create a feature branch.
3. Add or update tests/checks where possible.
4. Open a pull request with a clear summary.

Please follow [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).

## License

Licensed under [MIT](LICENSE).

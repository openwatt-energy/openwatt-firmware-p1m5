# P1M5 Dongle Firmware

DSMR P1 dongle firmware: reads smart meter via P1 port, serves Web UI + REST API, optional MQTT client.

Built with **PlatformIO + Arduino** (ESP32), not ESPHome.

## Features

- **P1**: Serial2 GPIO 16/17, 115200 8N1, DSMR telegram parsing
- **Web**: Async HTTP server, WebSocket live stream, REST API
- **MQTT**: Optional TLS publish to `mqtt.example.com`
- **OTA**: Optional over-the-air updates

## Quick Start

```bash
# Test Web UI locally (no hardware)
cd web_simulator && python3 server.py

# Run unit tests
pio test -e test

# Build firmware
pio run

# Flash to device
./build-and-flash.sh
```

## Full Documentation

See **[DEV.md](DEV.md)** for complete guide covering:

- Web UI testing (local simulator)
- Unit testing (PlatformIO)
- Building firmware
- Flashing (local/remote/OTA)
- Deployment process
- API testing
- Debugging & monitoring
- Troubleshooting

See **[MQTT_API.md](MQTT_API.md)** for complete documentation covering the bidirectional MQTT capabilities, including:
- HTTP Proxy mapping
- Modbus Proxy querying
- Local Network Scanner
- System Configuration Commands

## Configuration

Feature flags are set in `src/config.h`. Secrets (the SALT and MQTT broker host)
are injected at build time — see **[SECURITY.md](SECURITY.md)**.

WiFi/MQTT settings via Web UI at `http://<device-ip>/settings`

## Project Structure

```
├── src/              # Source code
├── data/             # Web UI files
├── test/             # Unit tests & P1 simulator
├── web_simulator/    # Local web UI testing
└── DEV.md            # Development documentation
```

## License

Apache License 2.0 — see [LICENSE](LICENSE).

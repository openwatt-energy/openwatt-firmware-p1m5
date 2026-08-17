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

## Build with Docker

No local PlatformIO/toolchain needed:

```bash
docker compose run --rm build    # build firmware
docker compose run --rm shell    # interactive shell in the build env
```

Note: the unit tests (`pio test -e test`) run on real hardware, so they
require a connected ESP32 and are not run in Docker/CI.

Built binaries land in `.pio/build/<env>/` on the host.

## User Documentation

See **[docs/](docs/)** for user-facing guides:

- [User Manual](docs/USER_MANUAL.md) — what it does and how to install it
- [LED Indicator](docs/LED_INDICATOR.md) — LED colour/blink meanings
- [Hardware](docs/HARDWARE.md) — pinout, meter connection, buttons & reset

## Full Documentation

See **[DEVELOPMENT.md](DEVELOPMENT.md)** for complete guide covering:

- Web UI testing (local simulator)
- Unit testing (PlatformIO)
- Building firmware
- Flashing (local/remote/OTA)
- Deployment process
- API testing
- Debugging & monitoring
- Troubleshooting

## Configuration

Edit `src/config.h`:

```cpp
#define FIRMWARE_VERSION "v1.0.45"
#define ENABLE_P1_READER 1    // P1 meter reading
#define ENABLE_MQTT 1         // MQTT client
#define ENABLE_OTA 1          // OTA updates
```

WiFi/MQTT settings via Web UI at `http://<device-ip>/settings`

## Project Structure

```
firmwares/p1m5/
├── src/              # Source code
├── data/             # Web UI files
├── test/             # Unit tests & P1 simulator
├── web_simulator/    # Local web UI testing
└── DEVELOPMENT.md    # Full documentation
```

## Reference Docs

For DSMR spec, UART layout, and API shape, see `firmware/` at repo root:

- `firmware/IMPLEMENTATION_SPEC.md` - UART, OBIS codes, telegram format, CRC16, API shapes
- `firmware/P1_PARSER_FINDINGS.md` - P1 parsing checklist
- `firmware/README.md` - Partition layout, memory segments

## Credits

The P1M5 dongle hardware was originally developed by **Re.alto** (an Elia
subsidiary, since closed). The project draws inspiration from the open-source
[plan-d-io P1-dongle](https://github.com/plan-d-io/P1-dongle) project.

## License

This project is licensed under the **Mozilla Public License 2.0 (MPL-2.0)** —
see [LICENSE](LICENSE).

Copyright © 2026 OpenWatt srl.

# OpenWatt P1 Reader (native firmware)

DSMR P1 dongle: reads smart meter via P1 port, serves Web UI + REST API, optional MQTT client. Built with **Espressif (PlatformIO + Arduino)**, not ESPHome.

- **P1**: Serial2 GPIO 16/17, 115200 8N1, DSMR telegram parsing
- **Web**: Async HTTP server, WebSocket live stream, REST API (settings, system, live)
- **MQTT**: Optional TLS publish to `mqtt.example.com` (enable in `src/config.h`)
- **OTA**: Optional (enable in `src/config.h`)

## Build & flash

```bash
cd openwatt-p1-reader
python3 -m platformio run
./build-and-flash.sh                    # local (auto-detects port)
./build-and-flash.sh /dev/cu.usbserial-xxx
./build-and-flash.sh remote             # copy + flash via remote host (env: REMOTE_HOST, REMOTE_SSH_PORT, REMOTE_KEY, REMOTE_SERIAL)
# Example: serial on remote at ttyUSB1, SSH on port 2222 with key:
#   REMOTE_HOST=127.0.0.1 REMOTE_SSH_PORT=2222 REMOTE_KEY=~/.ssh/openwatt REMOTE_SERIAL=/dev/ttyUSB1 ./build-and-flash.sh remote
#
# Remote bootloader (current FW has no API): on the remote host run scripts/enter_bootloader.py to toggle DTR/RTS, then flash:
#   scp scripts/enter_bootloader.py remote:/tmp/ && ssh remote "python3 /tmp/enter_bootloader.py /dev/ttyUSB1"
#   Then within a few seconds: ./build-and-flash.sh remote
# OTA (device must be on WiFi, listens on port 3232):
python3 -m platformio run -t upload -e m5stack-core-esp32 --upload-port <device-ip>

# OTA over SSH tunnel (device not on your LAN): forward 3232, reverse 28214 (host port for firmware pull)
#   ssh -L 3232:<device-ip>:3232 -R 28214:127.0.0.1:28214 user@gateway
#   python3 -m platformio run -t upload -e m5stack-core-esp32 --upload-port 127.0.0.1
```

**Serial console** (115200 8N1): `./scripts/monitor.sh /dev/tty.usbserial-XXX` or `python3 -m platformio device monitor --port /dev/cu.usbserial-XXX`. On macOS the port is `/dev/cu.usbserial-*` or `/dev/tty.usbserial-*` (dot, not hyphen). If you see garbage, ensure baud is 115200.

## Versioning

Firmware version is in `src/config.h` as `FIRMWARE_VERSION` (shown in UI and `/api/system`). Bump on release: **minor** = new features, **patch** = fixes. Builds are not tagged automatically; tag in git when you release (e.g. `git tag v1.1.0`).

## Config

- **Feature flags**: `src/config.h` — `ENABLE_P1_READER`, `ENABLE_MQTT`, `ENABLE_OTA`
- **WiFi/MQTT**: Via Web UI (AP at 192.168.4.1) or REST API; stored in NVS

## Reference (reverse‑engineered Xenn firmware)

For DSMR behaviour, UART layout, and API shape, see the **`firmware/`** folder at repo root:

- **`firmware/IMPLEMENTATION_SPEC.md`** — UART (Serial2 GPIO 16/17, 115200 8N1), OBIS codes, telegram format, CRC16, JSON/MQTT/WebSocket shapes, NVS keys
- **`firmware/P1_PARSER_FINDINGS.md`** — P1 parsing checklist and OBIS list
- **`firmware/README.md`** — Partition layout (nvs @ 0x9000, app0 @ 0x10000, spiffs), memory segments, API endpoints

This firmware follows that spec: same P1 pins/baud, same 12 OBIS codes, same REST/WS JSON layout.

## Tests

```bash
python3 -m platformio test -e test
```

See `QUICK_START.md` and `TESTING.md` for details.

## Simulate with Wokwi (no hardware)

Build, then run the firmware in [Wokwi](https://wokwi.com) or the **Wokwi for VS Code** extension for fast API/UI testing:

1. `python3 -m platformio run -e m5stack-core-esp32`
2. F1 → **Wokwi: Start Simulator** (or open wokwi.com and load `diagram.json` + firmware)
3. When the sim is running: `python3 scripts/wokwi_test.py` to run HTTP API checks automatically

Details: `wokwi/README.md`

---

**ESPHome**: The `esphome/` folder is an alternative/experimental ESPHome-based build. The **recommended and maintained** firmware is this native PlatformIO build (this directory).

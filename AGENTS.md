# AGENTS.md — P1M5 Firmware

> See `openwatt-infra/AGENTS.md` for the global OpenWatt v2 agent playbook.
> This file covers firmware-specific conventions.

---

## Repo

`openwatt-firmware-p1m5` — P1M5 dongle firmware (ESP32 / PlatformIO / Arduino).

---

## Build System

- `platformio.ini` — environments: `openwatt` (default), `soliseco`
- `src/customer_config.py` — generates `src/customer_config.h` from
  `src/customers/<env>.json` (run as PlatformIO pre-script, or standalone:
  `python3 src/customer_config.py src/customers/openwatt.json`)
- `scripts/build_templates.py` — compiles HTML/CSS templates into C++ headers
- `scripts/rename_firmware.py` — renames output to `p1m5_<version>_<variant>.bin`
- `scripts/build_secrets.py` — injects secrets at build time (see §Secrets)

## Customer Config

Each customer has a JSON file in `src/customers/`:

```json
{
  "customer": "openwatt",
  "display_name": "OpenWatt",
  "version_suffix": "ow",
  "ap_ssid_prefix": "OpenWatt",
  "salt_string": "CHANGE_ME_SALT",
  "mqtt": { "broker_host": "mqtt.openwatt.eu", "broker_port": 8883 },
  "ota": { "firmware_url": "https://api.openwatt.eu/ota" },
  "features": { "json_api_enabled": true, "mqtt_settings_ui": false },
  "theme": { "primary": "#2563eb", ... }
}
```

The `ota.firmware_url` field defaults to `api.openwatt.eu/ota` if omitted.

## Coding Conventions

- Indentation: 4 spaces
- Quotes: single quotes (C++), double quotes (JSON/Python strings)
- Line length: 88 characters
- C++: use `#ifndef` guards for build-time-overridable defines

## Secrets

- `SALT_STRING`, `MQTT_BROKER_HOST`, and `OTA_FIRMWARE_URL` are injected at
  build time via customer JSON or CI secrets.
- The repo ships with `CHANGE_ME_SALT` / `mqtt.example.com` / `api.openwatt.eu/ota`
  as defaults.
- **Never commit real credentials.**

## Releasing

1. Create a branch `release/vX.Y.Z-rcN` from the target base
2. Bump `FIRMWARE_VERSION_BASE` in `src/config.h`
3. Follow `docs/PRE_RELEASE_REVIEW.md` checklist
4. Build: `pio run -e openwatt`
5. Verify the binary: `strings p1m5_vX.Y.Z-rcN-ow.bin | grep "api\."`
6. Create a GitHub Release with `p1m5_v*.bin`, `bootloader.bin`, `partitions.bin`
7. Tag: `vX.Y.Z-rcN`
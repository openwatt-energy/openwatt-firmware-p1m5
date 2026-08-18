# Customer Firmware Builds

Customer-specific configuration lives in `src/customers/<customer>.json`. A
single generator, `src/customer_config.py`, converts that JSON into
`src/customer_config.h` at build time — no manual step required.

Each PlatformIO environment name maps directly to a JSON file of the same name
(`openwatt` → `src/customers/openwatt.json`, etc.).

## Build a variant

```bash
pio run -e openwatt    # OpenWatt (default)
pio run -e soliseco    # SolisEco
pio run -e creos       # Creos
```

`src/customer_config.py` runs automatically (wired as an extra_script in
`platformio.ini`) and generates `src/customer_config.h` from the matching JSON
before compilation.

## Creating a new customer

1. Create `src/customers/<customer>.json`:

```json
{
  "customer": "customername",
  "display_name": "Customer Name",
  "version_suffix": "customername",
  "fingerprint_default": "identifier",
  "ap_ssid_prefix": "CustomerPrefix",
  "salt_string": "CHANGE_ME_SALT",
  "mqtt": {
    "broker_host": "mqtt.example.com",
    "broker_port": 8883,
    "topic_prefix": "prefix/",
    "publish_interval_ms": 5000
  },
  "features": {
    "json_api_enabled": true,
    "mqtt_settings_ui": false
  },
  "theme": {
    "primary": "#ff0000",
    "background": "#ffffff",
    "text": "#000000",
    "accent": "#cc0000"
  }
}
```

2. Add an environment to `platformio.ini` (copy the `[env:soliseco]` block and
   change the name and `CUSTOMER_ID`):

```ini
[env:customername]
platform = espressif32
board = m5stack-atom
framework = arduino
extra_scripts = pre:scripts/build_templates.py, src/customer_config.py, scripts/rename_firmware.py
build_src_filter = +<*> -<arduino_main_stub.cpp>
lib_deps =
  knolleary/PubSubClient@^2.8
  bblanchon/ArduinoJson@^7.3.0
  me-no-dev/ESPAsyncWebServer@^1.2.4
  me-no-dev/AsyncTCP@^1.1.1
  fastled/FastLED@^3.9.4
build_flags =
  -DCORE_DEBUG_LEVEL=3
  -DARDUINO_RUNNING_CORE=1
  -DCUSTOMER_CONFIG_H
  -DCUSTOMER_ID=customername
```

3. Build:

```bash
pio run -e customername
```

## Configuration reference

| Field | Type | Description |
|-------|------|-------------|
| `customer` | string | Internal customer ID (no spaces) |
| `display_name` | string | Display name in UI |
| `version_suffix` | string | Firmware version suffix (e.g. `ow`, `soliseco`) |
| `fingerprint_default` | string | Default fingerprint (can be overridden in NVS) |
| `ap_ssid_prefix` | string | Prefix for AP SSID (e.g. `SolisEco` → `SolisEco-P1AABBCC`) |
| `salt_string` | string | Salt used for MQTT password derivation |
| `mqtt.broker_host` | string | MQTT broker hostname |
| `mqtt.broker_port` | int | MQTT broker port (8883 for TLS) |
| `mqtt.topic_prefix` | string | MQTT topic prefix |
| `mqtt.publish_interval_ms` | int | MQTT publish interval in milliseconds |
| `features.json_api_enabled` | bool | Enable JSON API endpoints |
| `features.mqtt_settings_ui` | bool | Show MQTT settings in web UI |
| `theme.primary` | string | Primary color (hex) |
| `theme.background` | string | Background color (hex) |
| `theme.text` | string | Text color (hex) |
| `theme.accent` | string | Accent color (hex) |

## NVS runtime overrides

The following values can be set at runtime via MQTT commands or API:

| Key | Type | Description |
|-----|------|-------------|
| `customer_fingerprint` | string | Override fingerprint |
| `mqtt_host` | string | Override MQTT broker |
| `mqtt_interval` | int | Override publish interval |

## Reboot counter

The firmware tracks the number of reboots in NVS. This counter is:
- Incremented on each boot
- Sent in MQTT status messages (`/sys/config` topic)
- Displayed on the System page

## Firmware version

The base version is `FIRMWARE_VERSION_BASE` in `src/config.h`; the customer
suffix (from `version_suffix`) is appended automatically. Current base:
`v1.0.48-rc1`.

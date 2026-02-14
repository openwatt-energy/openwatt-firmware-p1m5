# Customer Firmware Builds

This directory contains customer-specific configuration for building variant firmware images.

## Quick Start

### Build for SolisEco

```bash
# Generate customer config header from JSON
python3 src/customer_config.py src/customers/soliseco.json

# Build the firmware
pio run -e soliseco
```

### Build for OpenWatt (default)

```bash
# Build without customer config (uses defaults)
pio run -e m5stack-atom
```

## Creating a New Customer

1. Create a new JSON config file in `src/customers/<customer>.json`:

```json
{
  "customer": "customername",
  "display_name": "Customer Name",
  "fingerprint_default": "identifier",
  "ap_ssid_prefix": "CustomerPrefix",
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

2. Add the environment to `platformio.ini`:

```ini
[env:customername]
extends = env:m5stack-atom
build_flags =
  ${env.m5stack-atom.build_flags}
  -DCUSTOMER_CONFIG_H
```

3. Build:

```bash
python3 src/customer_config.py src/customers/customername.json
pio run -e customername
```

## Configuration Reference

| Field | Type | Description |
|-------|------|-------------|
| `customer` | string | Internal customer ID (no spaces) |
| `display_name` | string | Display name in UI |
| `fingerprint_default` | string | Default fingerprint (can be overridden in NVS) |
| `ap_ssid_prefix` | string | Prefix for AP SSID (e.g., "SolisEco" → "SolisEcoP1AABBCC") |
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

## NVS Runtime Overrides

The following values can be set at runtime via MQTT commands or API:

| Key | Type | Description |
|-----|------|-------------|
| `customer_fingerprint` | string | Override fingerprint |
| `mqtt_host` | string | Override MQTT broker |
| `mqtt_interval` | int | Override publish interval |

## Reboot Counter

The firmware tracks the number of reboots in NVS. This counter is:
- Incremented on each boot
- Sent in MQTT status messages (`/sys/config` topic)
- Displayed on the System page

## Firmware Version

- OpenWatt: `v1.0.24-ow`
- SolisEco: `v1.0.24-soliseco`

The version suffix is automatically appended based on the customer config.

#!/usr/bin/env python3
"""
Customer config generator.

Converts a customer JSON config to a C header for compile-time inclusion.

Two ways to run:

1. Standalone CLI:
       python3 customer_config.py src/customers/soliseco.json
   Output: src/customer_config.h (gitignored)

2. As a PlatformIO extra_script (wired in platformio.ini): the customer is
   derived from the build environment name (PIOENV) and the header is
   generated automatically before compilation.
"""

import json
import re
import sys
from pathlib import Path


def sanitize_c_name(name):
    """Convert string to valid C identifier."""
    return re.sub(r'[^a-zA-Z0-9_]', '_', name)


def json_to_c_value(value, value_type=None):
    """Convert JSON value to C literal."""
    if isinstance(value, bool):
        return '1' if value else '0'
    elif isinstance(value, int):
        return str(value)
    elif isinstance(value, float):
        return str(value)
    elif isinstance(value, str):
        escaped = value.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
        return f'"{escaped}"'
    return None


def generate_header(config, customer_name):
    """Generate C header from customer config."""

    version_suffix = config.get("version_suffix", customer_name)

    lines = [
        "/* Auto-generated file - DO NOT EDIT */",
        "/* Generated from: {}.json */".format(customer_name),
        "",
        f'#define CUSTOMER_NAME "{config.get("customer", "")}"',
        f'#define CUSTOMER_DISPLAY_NAME "{config.get("display_name", "")}"',
        f'#define CUSTOMER_FINGERPRINT_DEFAULT "{config.get("fingerprint_default", "")}"',
        "",
        "// SALT_STRING (MQTT shared secret) - overridable at build time",
        "#ifndef SALT_STRING",
        f'#define SALT_STRING "{config.get("salt_string", "")}"',
        "#endif",
        "",
        "// AP Configuration",
        f'#define AP_SSID_PREFIX "{config.get("ap_ssid_prefix", "OpenWatt")}"',
        "",
        "// MQTT Configuration",
    ]

    mqtt = config.get("mqtt", {})
    lines.extend([
        "#ifndef MQTT_BROKER_HOST",
        f'#define MQTT_BROKER_HOST "{mqtt.get("broker_host", "mqtt.example.com")}"',
        "#endif",
        f'#define MQTT_BROKER_PORT {mqtt.get("broker_port", 8883)}',
        f'#define MQTT_DEFAULT_TOPIC "{mqtt.get("topic_prefix", "P1M5/")}"',
        f'#define MQTT_PUBLISH_INTERVAL_MS {mqtt.get("publish_interval_ms", 5000)}',
        "",
        "// Feature Flags",
    ])

    features = config.get("features", {})
    lines.extend([
        f'#define JSON_API_ENABLED {1 if features.get("json_api_enabled", True) else 0}',
        f'#define MQTT_SETTINGS_UI_ENABLED {1 if features.get("mqtt_settings_ui", False) else 0}',
        "",
        "// Theme Colors (CSS hex values)",
    ])

    theme = config.get("theme", {})
    lines.extend([
        f'#define THEME_PRIMARY "{theme.get("primary", "#2563eb")}"',
        f'#define THEME_BACKGROUND "{theme.get("background", "#ffffff")}"',
        f'#define THEME_TEXT "{theme.get("text", "#111827")}"',
        f'#define THEME_ACCENT "{theme.get("accent", "#2563eb")}"',
        "",
        "// Firmware version suffix",
        f'#define FIRMWARE_VERSION_SUFFIX "-{version_suffix}"',
    ])

    return '\n'.join(lines)


def generate_from_json(config_path):
    """Load a customer JSON and write src/customer_config.h."""
    config_path = Path(config_path)
    if not config_path.exists():
        print(f"Error: Config file not found: {config_path}")
        return False

    with open(config_path, 'r') as f:
        config = json.load(f)

    customer_name = config.get('customer', 'unknown')
    header_content = generate_header(config, customer_name)

    output_path = config_path.parent.parent / 'customer_config.h'
    with open(output_path, 'w') as f:
        f.write(header_content)

    print(f"Generated: {output_path}")
    print(f"Customer: {config.get('display_name')} ({customer_name})")
    return True


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 customer_config.py <config.json>")
        sys.exit(1)
    generate_from_json(sys.argv[1])


# PlatformIO extra_script mode: derive the customer from the build env name.
try:
    Import("env")
    pio_env = env.get("PIOENV", "")
    if pio_env and pio_env != "test":
        project_dir = Path(env.get("PROJECT_DIR", "."))
        config_path = project_dir / "src" / "customers" / f"{pio_env}.json"
        if config_path.exists():
            generate_from_json(config_path)
except Exception:
    pass

if __name__ == "__main__":
    main()

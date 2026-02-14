#!/usr/bin/env python3
"""
Customer Config Generator

Converts customer JSON config to C header file for compile-time inclusion.

Usage:
    python3 customer_config.py src/customers/soliseco.json

Output:
    src/customer_config.h (generated, gitignored)
"""

import json
import sys
import re
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
        # Escape special characters for C string
        escaped = value.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
        return f'"{escaped}"'
    elif isinstance(value, dict):
        return None  # Handle separately
    elif isinstance(value, list):
        return None  # Handle separately
    return None


def generate_header(config, customer_name):
    """Generate C header from customer config."""

    lines = [
        "/* Auto-generated file - DO NOT EDIT */",
        "/* Generated from: {}.json */".format(customer_name),
        "",
        f'#define CUSTOMER_NAME "{config.get("customer", "")}"',
        f'#define CUSTOMER_DISPLAY_NAME "{config.get("display_name", "")}"',
        f'#define CUSTOMER_FINGERPRINT_DEFAULT "{config.get("fingerprint_default", "")}"',
        f'#define SALT_STRING "{config.get("salt_string", "")}"',
        "",
        "// AP Configuration",
        f'#define AP_SSID_PREFIX "{config.get("ap_ssid_prefix", "OpenWatt")}"',
        "",
        "// MQTT Configuration",
    ]

    mqtt = config.get("mqtt", {})
    lines.extend([
        f'#define MQTT_BROKER_HOST "{mqtt.get("broker_host", "mqtt.example.com")}"',
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
        f'#define FIRMWARE_VERSION_SUFFIX "-{customer_name}"',
    ])

    return '\n'.join(lines)


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 customer_config.py <config.json>")
        sys.exit(1)

    config_path = Path(sys.argv[1])
    if not config_path.exists():
        print(f"Error: Config file not found: {config_path}")
        sys.exit(1)

    with open(config_path, 'r') as f:
        config = json.load(f)

    customer_name = config.get('customer', 'unknown')

    # Generate header content
    header_content = generate_header(config, customer_name)

    # Write to customer_config.h (in same directory as config)
    output_path = config_path.parent.parent / 'customer_config.h'

    with open(output_path, 'w') as f:
        f.write(header_content)

    print(f"Generated: {output_path}")
    print(f"Customer: {config.get('display_name')} ({customer_name})")
    print(f"Fingerprint: {config.get('fingerprint_default')}")
    print(f"MQTT: {config.get('mqtt', {}).get('broker_host')}")
    print(f"Theme: {config.get('theme', {}).get('primary')}")


if __name__ == '__main__':
    main()

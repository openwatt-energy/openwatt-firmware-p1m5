#!/usr/bin/env python3
"""
Build variant selector for OpenWatt P1 Reader firmware
Reads firmware_variants.json and builds selected variant
"""

import json
import sys
import os
from pathlib import Path

# Get project directory
if '__file__' in globals():
    PROJECT_DIR = Path(__file__).parent.parent
else:
    PROJECT_DIR = Path(os.getcwd())
VARIANTS_FILE = PROJECT_DIR / "firmware_variants.json"
CONFIG_HEADER = PROJECT_DIR / "src" / "variant_config.h"


def load_variants():
    """Load variant configurations from JSON"""
    with open(VARIANTS_FILE, 'r') as f:
        data = json.load(f)
    return data.get('variants', [])


def select_variant(variants):
    """Interactive variant selection"""
    print("\n=== OpenWatt Firmware Build System ===\n")
    print("Available variants:")
    print("-" * 60)

    for i, variant in enumerate(variants, 1):
        print(f"{i}. {variant['name']}")
        print(f"   Description: {variant['description']}")
        print(f"   Extension: {variant['fw_ext']}")
        print(f"   SSID: {variant['ssid_prefix']}-P1XXXXXX")
        print()

    print("-" * 60)

    while True:
        try:
            choice = input("Select variant (number or name): ").strip()

            # Try as number
            if choice.isdigit():
                idx = int(choice) - 1
                if 0 <= idx < len(variants):
                    return variants[idx]

            # Try as name
            for variant in variants:
                if variant['name'].lower() == choice.lower():
                    return variant

            print("Invalid selection. Please try again.")
        except (ValueError, IndexError):
            print("Invalid selection. Please enter a number or variant name.")


def generate_config_header(variant):
    """Generate C++ header file with variant configuration"""

    header_content = f"""// Auto-generated variant configuration
// Variant: {variant['name']}
// Description: {variant['description']}
// Generated: Do not edit manually

#ifndef VARIANT_CONFIG_H
#define VARIANT_CONFIG_H

// Firmware extension (used in filename)
#define FW_EXT "{variant['fw_ext']}"

// SSID prefix for AP mode
#define SSID_PREFIX "{variant['ssid_prefix']}"

// Feature flags
#define ALLOW_MQTT_CHANGES {'1' if variant.get('allow_mqtt_changes', True) else '0'}
#define ENABLE_MQTT {'1' if variant.get('enable_mqtt', True) else '0'}
#define ENABLE_OTA {'1' if variant.get('enable_ota', True) else '0'}
#define ENABLE_P1_READER {'1' if variant.get('enable_p1_reader', True) else '0'}

// UI Theme
#define UI_THEME "{variant.get('ui_theme', 'default')}"
"""

    # Add locked MQTT settings if MQTT changes are not allowed
    if not variant.get('allow_mqtt_changes', True) and variant.get('mqtt_host'):
        header_content += f"""
// Locked MQTT settings (when ALLOW_MQTT_CHANGES is 0)
#define LOCKED_MQTT_HOST "{variant.get('mqtt_host', 'mqtt.example.com')}"
#define LOCKED_MQTT_PORT {variant.get('mqtt_port', 8883)}
"""

    header_content += """
#endif // VARIANT_CONFIG_H
"""

    # Write header file
    with open(CONFIG_HEADER, 'w') as f:
        f.write(header_content)

    print(f"✓ Generated variant_config.h for '{variant['name']}'")
    return header_content


def set_env_for_pio(variant):
    """Set environment variables for PlatformIO build"""
    # Set output filename suffix
    os.environ['FW_EXT'] = variant['fw_ext']
    os.environ['PIO_ENV'] = variant['name']

    # Update PlatformIO env name if needed
    print(f"✓ Set build environment for variant '{variant['name']}'")


def build_variant(variant_name=None):
    """Main build function"""
    variants = load_variants()

    if not variants:
        print("Error: No variants found in firmware_variants.json")
        sys.exit(1)

    # Select variant
    if variant_name:
        # Find by name
        variant = None
        for v in variants:
            if v['name'].lower() == variant_name.lower():
                variant = v
                break
        if not variant:
            print(f"Error: Variant '{variant_name}' not found")
            sys.exit(1)
    else:
        variant = select_variant(variants)

    print(f"\nBuilding variant: {variant['name']}")
    print(f"Extension: {variant['fw_ext']}")
    print(f"SSID Prefix: {variant['ssid_prefix']}")
    print()

    # Generate config header
    generate_config_header(variant)

    # Set environment
    set_env_for_pio(variant)

    # Return variant info for PlatformIO
    return variant


if __name__ == "__main__":
    if len(sys.argv) > 1:
        # Build specific variant
        variant_name = sys.argv[1]
        build_variant(variant_name)
    else:
        # Interactive selection
        build_variant()

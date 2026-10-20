#!/usr/bin/env python3
"""
Build variant selector for OpenWatt firmware.
Reads firmware_variants.json and generates src/variant_config.h
for the selected variant.
"""

import json
import sys
import os
from pathlib import Path


# ============================================================================
# PATHS
# ============================================================================

if '__file__' in globals():
    PROJECT_DIR = Path(__file__).parent.parent
else:
    PROJECT_DIR = Path(os.getcwd())

VARIANTS_FILE = PROJECT_DIR / "firmware_variants.json"
CONFIG_HEADER = PROJECT_DIR / "src" / "variant_config.h"


# ============================================================================
# LOAD VARIANTS
# ============================================================================

def load_variants():
    """Load variant configurations from JSON."""

    with open(VARIANTS_FILE, 'r', encoding='utf-8') as f:
        data = json.load(f)

    return data.get('variants', [])


# ============================================================================
# SELECT VARIANT
# ============================================================================

def select_variant(variants):
    """Interactive variant selection."""

    print("\n=== OpenWatt Firmware Build System ===\n")
    print("Available variants:")
    print("-" * 60)

    for i, variant in enumerate(variants, 1):
        print(f"{i}. {variant['name']}")
        print(f"   Description: {variant['description']}")
        print(f"   Extension: {variant['fw_ext']}")
        print(f"   SSID: {variant['ssid_prefix']}-XXXXXX")

        if variant.get('enable_p1_reader', True):
            print("   Meter interface: P1")

        if variant.get('enable_linky_tic', False):
            print(
                "   Meter interface: Linky TIC "
                f"({variant.get('linky_mode', 'historique')})"
            )

        print()

    print("-" * 60)

    while True:
        try:
            choice = input(
                "Select variant (number or name): "
            ).strip()

            # Selection by number
            if choice.isdigit():
                idx = int(choice) - 1

                if 0 <= idx < len(variants):
                    return variants[idx]

            # Selection by name
            for variant in variants:
                if (
                    variant['name'].lower()
                    == choice.lower()
                ):
                    return variant

            print(
                "Invalid selection. Please try again."
            )

        except (ValueError, IndexError):
            print(
                "Invalid selection. "
                "Please enter a number or variant name."
            )


# ============================================================================
# GENERATE CONFIG HEADER
# ============================================================================

def generate_config_header(variant):
    """Generate C++ header file with variant configuration."""

    enable_p1_reader = variant.get(
        'enable_p1_reader',
        True
    )

    enable_linky_tic = variant.get(
        'enable_linky_tic',
        False
    )

    linky_mode = variant.get(
        'linky_mode',
        'historique'
    ).lower()

    # Safety check:
    # P1 and Linky must not use the same UART simultaneously.
    if enable_p1_reader and enable_linky_tic:
        print(
            "Error: ENABLE_P1_READER and ENABLE_LINKY_TIC "
            "cannot both be enabled."
        )
        sys.exit(1)

    linky_tic_standard = (
        enable_linky_tic
        and linky_mode == 'standard'
    )

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

// Meter interfaces
#define ENABLE_P1_READER {'1' if enable_p1_reader else '0'}
#define ENABLE_LINKY_TIC {'1' if enable_linky_tic else '0'}

// Linky TIC mode
// 0 = historique
// 1 = standard
#define LINKY_TIC_STANDARD {'1' if linky_tic_standard else '0'}

// UI Theme
#define UI_THEME "{variant.get('ui_theme', 'default')}"
"""

    # Add locked MQTT settings if MQTT changes are not allowed
    if (
        not variant.get(
            'allow_mqtt_changes',
            True
        )
        and variant.get('mqtt_host')
    ):
        header_content += f"""
// Locked MQTT settings
#define LOCKED_MQTT_HOST "{variant.get('mqtt_host', 'mqtt.example.com')}"
#define LOCKED_MQTT_PORT {variant.get('mqtt_port', 8883)}
"""

    header_content += """
#endif // VARIANT_CONFIG_H
"""

    with open(
        CONFIG_HEADER,
        'w',
        encoding='utf-8'
    ) as f:
        f.write(header_content)

    print(
        f"Generated variant_config.h "
        f"for '{variant['name']}'"
    )

    print(
        f"  ENABLE_P1_READER = "
        f"{1 if enable_p1_reader else 0}"
    )

    print(
        f"  ENABLE_LINKY_TIC = "
        f"{1 if enable_linky_tic else 0}"
    )

    if enable_linky_tic:
        print(
            f"  Linky mode = {linky_mode}"
        )

    return header_content


# ============================================================================
# PLATFORMIO ENVIRONMENT
# ============================================================================

def set_env_for_pio(variant):
    """Set environment variables for PlatformIO build."""

    os.environ['FW_EXT'] = variant['fw_ext']
    os.environ['PIO_ENV'] = variant['name']

    print(
        f"Set build environment for variant "
        f"'{variant['name']}'"
    )


# ============================================================================
# BUILD VARIANT
# ============================================================================

def build_variant(variant_name=None):
    """Main build function."""

    variants = load_variants()

    if not variants:
        print(
            "Error: No variants found "
            "in firmware_variants.json"
        )
        sys.exit(1)

    if variant_name:
        variant = None

        for v in variants:
            if (
                v['name'].lower()
                == variant_name.lower()
            ):
                variant = v
                break

        if not variant:
            print(
                f"Error: Variant "
                f"'{variant_name}' not found"
            )
            sys.exit(1)

    else:
        variant = select_variant(variants)

    print(
        f"\nBuilding variant: "
        f"{variant['name']}"
    )

    print(
        f"Extension: "
        f"{variant['fw_ext']}"
    )

    print(
        f"SSID Prefix: "
        f"{variant['ssid_prefix']}"
    )

    print()

    generate_config_header(variant)
    set_env_for_pio(variant)

    return variant


# ============================================================================
# MAIN
# ============================================================================

if __name__ == "__main__":

    if len(sys.argv) > 1:
        variant_name = sys.argv[1]
        build_variant(variant_name)

    else:
        build_variant()
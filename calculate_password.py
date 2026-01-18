#!/usr/bin/env python3
"""
Calculate WiFi AP password for OpenWatt P1 Reader
Password = MD5(DeviceID + SALT)
"""

import hashlib
import sys

SALT = "CHANGE_ME_SALT"

def calculate_password(device_id):
    """Calculate WiFi AP password from device ID"""
    combined = device_id + SALT
    md5_hash = hashlib.md5(combined.encode()).hexdigest()
    return md5_hash

if __name__ == "__main__":
    if len(sys.argv) > 1:
        device_id = sys.argv[1]
    else:
        print("Usage: python3 calculate_password.py <DeviceID>")
        print("Example: python3 calculate_password.py P1A1B2C3")
        print("\nTo find DeviceID:")
        print("  - Check serial output when device boots")
        print("  - DeviceID format: P1XXXXXX (last 6 hex chars of MAC)")
        sys.exit(1)
    
    password = calculate_password(device_id)
    print(f"Device ID: {device_id}")
    print(f"WiFi AP Password: {password}")
    print(f"\nSSID will be: OpenWatt-{device_id[2:]}")  # Remove "P1" prefix


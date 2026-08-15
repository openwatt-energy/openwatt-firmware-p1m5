#!/usr/bin/env python3
"""
Calculate WiFi AP password for OpenWatt P1 Reader.

Password = MD5(DeviceID + SALT)

The SALT must match the value compiled into the firmware (see src/config.h).
Provide it via the --salt argument or the OPENWATT_SALT environment variable.
"""

import argparse
import hashlib
import os
import sys


def calculate_password(device_id, salt):
    """Calculate WiFi AP password from device ID and salt."""
    return hashlib.md5((device_id + salt).encode()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description='Calculate WiFi AP password')
    parser.add_argument('device_id', help='DeviceID (e.g. P1A1B2C3)')
    parser.add_argument(
        '--salt',
        default=os.environ.get('OPENWATT_SALT', ''),
        help='SALT (default: $OPENWATT_SALT env var)',
    )
    args = parser.parse_args()

    if not args.salt:
        print('Error: no SALT provided. Use --salt or set OPENWATT_SALT.', file=sys.stderr)
        sys.exit(1)

    password = calculate_password(args.device_id, args.salt)
    print(f'Device ID: {args.device_id}')
    print(f'WiFi AP Password: {password}')
    # SSID strips the "P1" prefix from the DeviceID
    print(f'\nSSID will be: OpenWatt-{args.device_id[2:]}')


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""
Put ESP32 into serial bootloader by toggling DTR/RTS on the serial port.
Same sequence as esptool's ClassicReset (DTR=GPIO0, RTS=EN).
Run on the host that has the device on /dev/ttyUSBx, then run esptool within a few seconds.

Usage: python3 enter_bootloader.py [port]
  e.g. python3 enter_bootloader.py /dev/ttyUSB1

Requires: pyserial (pip install pyserial)
"""
import sys
import time

def main():
  port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
  try:
    import serial
  except ImportError:
    print("Need pyserial: pip install pyserial", file=sys.stderr)
    sys.exit(1)

  try:
    ser = serial.Serial(port, 115200)
  except Exception as e:
    print(f"Open {port}: {e}", file=sys.stderr)
    sys.exit(1)

  # ClassicReset: DTR -> GPIO0, RTS -> EN (inverted on many adapters)
  # Chip in reset (EN low): RTS=True, then release with DTR=True RTS=False -> GPIO0 low at boot
  ser.setDTR(False)   # IO0 high
  ser.setRTS(True)    # EN low, chip in reset
  time.sleep(0.1)
  ser.setDTR(True)    # IO0 low
  ser.setRTS(False)   # EN high, chip out of reset -> boots with GPIO0 low = bootloader
  time.sleep(0.05)
  ser.setDTR(False)   # IO0 high, done
  ser.close()
  print("Bootloader sequence sent. Run esptool within a few seconds.")

if __name__ == "__main__":
  main()

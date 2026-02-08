#!/bin/bash
# Monitor serial output without reflashing. Baud is 115200 (set in platformio.ini).
# On macOS the port is usually /dev/cu.usbserial-XXXX or /dev/tty.usbserial-XXXX (note: dot, not hyphen).

PORT=${1:-/dev/ttyUSB0}
BAUD=${2:-115200}

echo "Serial monitor: $PORT @ ${BAUD} (Ctrl+C to stop)"
if command -v pio &>/dev/null; then
  pio device monitor --port "$PORT" --baud "$BAUD"
else
  python3 -m platformio device monitor --port "$PORT" --baud "$BAUD"
fi

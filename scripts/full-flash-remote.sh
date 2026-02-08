#!/bin/bash
# Full flash (bootloader + partitions + app) for a device that had other firmware (e.g. ESPHome).
# Use when app-only flash leaves the device dead or no console/UI.
# Run from project root. Set REMOTE_* env vars as for build-and-flash.sh remote.

set -e
REMOTE_HOST="${REMOTE_HOST:?set REMOTE_HOST}"
REMOTE_SSH_PORT="${REMOTE_SSH_PORT:-22}"
REMOTE_SERIAL="${REMOTE_SERIAL:-/dev/ttyUSB1}"
REMOTE_USER="${REMOTE_USER:-root}"
REMOTE_SPEC="${REMOTE_USER}@${REMOTE_HOST}"
SSH_OPTS=(-o ConnectTimeout=10)
[[ -n "${REMOTE_KEY}" ]] && SSH_OPTS+=(-i "${REMOTE_KEY}")
[[ "${REMOTE_SSH_PORT}" != "22" ]] && SSH_OPTS+=(-p "${REMOTE_SSH_PORT}")
SCP_OPTS=(-o ConnectTimeout=10)
[[ -n "${REMOTE_KEY}" ]] && SCP_OPTS+=(-i "${REMOTE_KEY}")
[[ "${REMOTE_SSH_PORT}" != "22" ]] && SCP_OPTS+=(-P "${REMOTE_SSH_PORT}")

BUILD_DIR=".pio/build/m5stack-core-esp32"
BOOTLOADER="$BUILD_DIR/bootloader.bin"
PARTITIONS="$BUILD_DIR/partitions.bin"
FIRMWARE="$BUILD_DIR/firmware.bin"
for f in "$BOOTLOADER" "$PARTITIONS" "$FIRMWARE"; do
  [[ -f "$f" ]] || { echo "Missing $f. Run: python3 -m platformio run -e m5stack-core-esp32"; exit 1; }
done

echo "Copying bootloader + partitions + firmware to $REMOTE_SPEC..."
scp "${SCP_OPTS[@]}" "$BOOTLOADER" "$PARTITIONS" "$FIRMWARE" "${REMOTE_SPEC}:/tmp/"

echo "Put device in bootloader mode (hold BOOT, press RESET, release BOOT), then press ENTER..."
read -r

echo "Full flash: 0x1000 bootloader, 0x8000 partitions, 0x10000 firmware..."
ssh "${SSH_OPTS[@]}" "${REMOTE_SPEC}" "python3 -m esptool --chip esp32 --port $REMOTE_SERIAL --baud 57600 write_flash --flash_size 4MB --flash_mode dio 0x1000 /tmp/bootloader.bin 0x8000 /tmp/partitions.bin 0x10000 /tmp/firmware.bin"

echo "Done. Power cycle or press RESET; connect serial at 115200 to see output."

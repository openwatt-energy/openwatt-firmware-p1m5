#!/bin/bash
# Build and flash OpenWatt P1 Reader firmware

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}🔨 Building OpenWatt P1 Reader firmware...${NC}"

# Build the firmware (try pio first, fallback to python3 -m platformio)
if command -v pio &> /dev/null; then
  pio run
else
  python3 -m platformio run
fi

if [ $? -ne 0 ]; then
  echo -e "${RED}❌ Build failed!${NC}"
  exit 1
fi

echo -e "${GREEN}✅ Build successful!${NC}"
echo ""
echo -e "${YELLOW}📦 Firmware binary location:${NC}"
echo "   .pio/build/m5stack-core-esp32/firmware.bin"
echo ""

FIRMWARE_BIN=".pio/build/m5stack-core-esp32/firmware.bin"
REMOTE_HOST="piflash"
REMOTE_PATH="/tmp/firmware.bin"
REMOTE_PORT="/dev/ttyUSB0"

# Check if remote flash is requested
if [ "$1" == "remote" ] || [ "$1" == "piflash" ]; then
  echo -e "${GREEN}📤 Copying firmware to ${REMOTE_HOST}...${NC}"
  
  # Copy firmware to remote Pi
  scp "$FIRMWARE_BIN" "${REMOTE_HOST}:${REMOTE_PATH}"
  
  if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Failed to copy firmware to ${REMOTE_HOST}!${NC}"
    exit 1
  fi
  
  echo -e "${GREEN}✅ Firmware copied to ${REMOTE_HOST}:${REMOTE_PATH}${NC}"
  echo ""
  echo -e "${YELLOW}⚠️  Put ESP32 in bootloader mode NOW:${NC}"
  echo "   1. Hold BOOT button"
  echo "   2. Press and release RESET button"
  echo "   3. Release BOOT button"
  echo ""
  read -p "Press ENTER when ready to flash..."
  echo ""
  echo -e "${GREEN}📤 Flashing firmware via ${REMOTE_HOST}...${NC}"
  
  # Flash via remote Pi (try esptool.py first, fallback to esptool)
  # Use lower baud rate and add retry
  MAX_RETRIES=3
  RETRY_COUNT=0
  
  while [ $RETRY_COUNT -lt $MAX_RETRIES ]; do
    if ssh "${REMOTE_HOST}" "if command -v esptool.py &> /dev/null; then esptool.py --baud 115200 --chip esp32 --port ${REMOTE_PORT} write_flash 0x10000 ${REMOTE_PATH}; else esptool --baud 115200 --chip esp32 --port ${REMOTE_PORT} write_flash 0x10000 ${REMOTE_PATH}; fi" 2>&1; then
      FLASH_SUCCESS=true
      break
    else
      RETRY_COUNT=$((RETRY_COUNT + 1))
      if [ $RETRY_COUNT -lt $MAX_RETRIES ]; then
        echo -e "${YELLOW}⚠️  Flash failed, retrying ($RETRY_COUNT/$MAX_RETRIES)...${NC}"
        echo -e "${YELLOW}   Put device in bootloader mode again and press ENTER...${NC}"
        read -p ""
      fi
    fi
  done
  
  if [ "$FLASH_SUCCESS" = true ]; then
    echo -e "${GREEN}✅ Flash successful!${NC}"
    # Clean up remote file
    ssh "${REMOTE_HOST}" "rm -f ${REMOTE_PATH}"
  else
    echo -e "${RED}❌ Flash failed after ${MAX_RETRIES} attempts!${NC}"
    echo -e "${YELLOW}💡 Troubleshooting:${NC}"
    echo "   1. Ensure ESP32 is in bootloader mode (hold BOOT, press RESET, release BOOT)"
    echo "   2. Check USB connection: ssh ${REMOTE_HOST} 'ls -l ${REMOTE_PORT}'"
    echo "   3. Try manually: ssh ${REMOTE_HOST} 'esptool.py --baud 115200 --chip esp32 --port ${REMOTE_PORT} write_flash 0x10000 ${REMOTE_PATH}'"
    echo "   4. Check if device is detected: ssh ${REMOTE_HOST} 'dmesg | tail -20'"
    exit 1
  fi
  
  exit 0
fi

# Check if port is provided
if [ -z "$1" ]; then
  # Try to auto-detect first usbserial port
  AUTO_PORT=$(ls /dev/cu.usbserial* 2>/dev/null | head -n 1)
  
  if [ -n "$AUTO_PORT" ]; then
    echo -e "${YELLOW}🔍 Auto-detected port: ${AUTO_PORT}${NC}"
    PORT="$AUTO_PORT"
  else
    echo -e "${YELLOW}💡 To flash the firmware, run:${NC}"
    echo "   $0 /dev/cu.usbserial-XXXXX    (local flash)"
    echo "   $0 remote                     (flash via piflash)"
    echo ""
    echo -e "${YELLOW}Available serial ports:${NC}"
    ls /dev/cu.* 2>/dev/null || echo "   (none found)"
    echo ""
    echo -e "${YELLOW}Or flash manually with esptool:${NC}"
    echo "   esptool --chip esp32 --port PORT write-flash 0x10000 .pio/build/m5stack-core-esp32/firmware.bin"
    exit 0
  fi
else
  PORT=$1
fi

echo -e "${GREEN}📤 Flashing firmware to $PORT...${NC}"

# Flash the firmware
esptool --chip esp32 --port "$PORT" write-flash 0x10000 .pio/build/m5stack-core-esp32/firmware.bin

if [ $? -eq 0 ]; then
  echo -e "${GREEN}✅ Flash successful!${NC}"
else
  echo -e "${RED}❌ Flash failed!${NC}"
  echo -e "${YELLOW}💡 Troubleshooting:${NC}"
  echo "   1. Put device in bootloader mode (hold BOOT, press RESET, release BOOT)"
  echo "   2. Check if port is busy: lsof | grep $PORT"
  echo "   3. Try a different port: ls /dev/cu.*"
  exit 1
fi


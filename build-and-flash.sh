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
REMOTE_PATH="/tmp/firmware.bin"
# Remote serial flash: override with env (e.g. REMOTE_HOST=127.0.0.1 REMOTE_SSH_PORT=2222 REMOTE_KEY=~/.ssh/openwatt REMOTE_SERIAL=/dev/ttyUSB1)
REMOTE_HOST="${REMOTE_HOST:-piflash}"
REMOTE_SSH_PORT="${REMOTE_SSH_PORT:-22}"
REMOTE_SERIAL="${REMOTE_SERIAL:-/dev/ttyUSB0}"
SSH_OPTS=(-o ConnectTimeout=10)
[[ -n "${REMOTE_KEY}" ]] && SSH_OPTS+=(-i "${REMOTE_KEY}")
[[ "${REMOTE_SSH_PORT}" != "22" ]] && SSH_OPTS+=(-p "${REMOTE_SSH_PORT}")
SCP_OPTS=(-o ConnectTimeout=10)
[[ -n "${REMOTE_KEY}" ]] && SCP_OPTS+=(-i "${REMOTE_KEY}")
[[ "${REMOTE_SSH_PORT}" != "22" ]] && SCP_OPTS+=(-P "${REMOTE_SSH_PORT}")
REMOTE_USER="${REMOTE_USER:-root}"
REMOTE_SPEC="${REMOTE_USER}@${REMOTE_HOST}"

# Check if network (TCP) flash is requested: host:port e.g. 127.0.0.1:3232 or 0.0.0.0:3232
if [[ "$1" =~ ^[0-9.]+:[0-9]+$ ]] || [[ "$1" =~ ^[a-zA-Z0-9._-]+:[0-9]+$ ]]; then
  HOST="${1%%:*}"
  PORT="${1##*:}"
  # Connect to 127.0.0.1 when server binds 0.0.0.0
  if [ "$HOST" = "0.0.0.0" ]; then
    HOST="127.0.0.1"
  fi
  echo -e "${GREEN}📤 Flashing via network to ${HOST}:${PORT} (socket)...${NC}"
  echo -e "${YELLOW}   Put ESP32 in bootloader mode first (hold BOOT, press RESET, release BOOT).${NC}"
  echo ""
  if command -v esptool.py &> /dev/null; then
    esptool.py --chip esp32 --port "socket://${HOST}:${PORT}" write_flash 0x10000 "$FIRMWARE_BIN"
  else
    python3 -m esptool --chip esp32 --port "socket://${HOST}:${PORT}" write_flash 0x10000 "$FIRMWARE_BIN"
  fi
  if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ Flash successful!${NC}"
  else
    echo -e "${RED}❌ Flash failed!${NC}"
    echo "   Raw socket has no DTR/RTS: put device in bootloader mode before running."
    exit 1
  fi
  exit 0
fi

# Check if remote flash is requested
if [ "$1" == "remote" ] || [ "$1" == "piflash" ]; then
  echo -e "${GREEN}📤 Copying firmware to ${REMOTE_SPEC}...${NC}"
  
  scp "${SCP_OPTS[@]}" "$FIRMWARE_BIN" "${REMOTE_SPEC}:${REMOTE_PATH}"
  
  if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Failed to copy firmware to ${REMOTE_SPEC}!${NC}"
    exit 1
  fi
  
  echo -e "${GREEN}✅ Firmware copied to ${REMOTE_SPEC}:${REMOTE_PATH}${NC}"
  echo ""

  # Optionally trigger bootloader via DTR/RTS on the serial port (for remote device with no API)
  if [[ -z "${REMOTE_SKIP_BOOTLOADER_SCRIPT}" ]] && [[ -f "scripts/enter_bootloader.py" ]]; then
    echo -e "${GREEN}📤 Sending DTR/RTS bootloader sequence on ${REMOTE_SERIAL}...${NC}"
    scp "${SCP_OPTS[@]}" scripts/enter_bootloader.py "${REMOTE_SPEC}:/tmp/enter_bootloader.py" 2>/dev/null
    ssh "${SSH_OPTS[@]}" "${REMOTE_SPEC}" "python3 /tmp/enter_bootloader.py ${REMOTE_SERIAL}" 2>/dev/null || true
    sleep 1
  else
    echo -e "${YELLOW}⚠️  Put ESP32 in bootloader mode (or set REMOTE_SKIP_BOOTLOADER_SCRIPT=1 if already in bootloader):${NC}"
    echo "   On remote host run: python3 /tmp/enter_bootloader.py ${REMOTE_SERIAL}"
    echo ""
    read -p "Press ENTER when ready to flash..."
  fi
  echo ""
  echo -e "${GREEN}📤 Flashing firmware via ${REMOTE_SPEC} (${REMOTE_SERIAL})...${NC}"
  
  MAX_RETRIES=3
  RETRY_COUNT=0
  
  while [ $RETRY_COUNT -lt $MAX_RETRIES ]; do
    if ssh "${SSH_OPTS[@]}" "${REMOTE_SPEC}" "if command -v esptool.py &> /dev/null; then esptool.py --baud 115200 --chip esp32 --port ${REMOTE_SERIAL} --flash_size 4MB --flash_mode dio write_flash 0x10000 ${REMOTE_PATH}; else esptool --baud 115200 --chip esp32 --port ${REMOTE_SERIAL} write_flash -z --flash_size 4MB 0x10000 ${REMOTE_PATH}; fi" 2>&1; then
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
    ssh "${SSH_OPTS[@]}" "${REMOTE_SPEC}" "rm -f ${REMOTE_PATH}"
  else
    echo -e "${RED}❌ Flash failed after ${MAX_RETRIES} attempts!${NC}"
    echo -e "${YELLOW}💡 Troubleshooting:${NC}"
    echo "   1. Ensure ESP32 is in bootloader mode (hold BOOT, press RESET, release BOOT)"
    echo "   2. Check USB: ssh ${SSH_OPTS[*]} ${REMOTE_SPEC} 'ls -l ${REMOTE_SERIAL}'"
    echo "   3. Try manually: ssh ${SSH_OPTS[*]} ${REMOTE_SPEC} 'esptool.py --baud 115200 --chip esp32 --port ${REMOTE_SERIAL} write_flash 0x10000 ${REMOTE_PATH}'"
    exit 1
  fi
  
  exit 0
fi

# Check if port is provided
if [ -z "$1" ]; then
  # Try to auto-detect first usbserial port (cu or tty on macOS)
  AUTO_PORT=$(ls /dev/cu.usbserial* 2>/dev/null | head -n 1)
  [[ -z "$AUTO_PORT" ]] && AUTO_PORT=$(ls /dev/tty.usbserial* 2>/dev/null | head -n 1)
  
  if [ -n "$AUTO_PORT" ]; then
    echo -e "${YELLOW}🔍 Auto-detected port: ${AUTO_PORT}${NC}"
    PORT="$AUTO_PORT"
  else
    echo -e "${YELLOW}💡 To flash the firmware, run:${NC}"
    echo "   $0 /dev/tty.usbserial-XXX     (local flash)"
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

# Flash the firmware (esptool.py preferred)
if command -v esptool.py &> /dev/null; then
  esptool.py --chip esp32 --port "$PORT" write_flash 0x10000 "$FIRMWARE_BIN"
else
  esptool --chip esp32 --port "$PORT" write-flash 0x10000 "$FIRMWARE_BIN"
fi

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


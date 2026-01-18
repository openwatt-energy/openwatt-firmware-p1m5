#!/bin/bash
# Local testing options for ESP32 firmware

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}Local Testing Options for ESP32 Firmware${NC}"
echo ""
echo "1. ${YELLOW}Compile-only verification${NC} (no hardware needed):"
echo "   python3 -m platformio run"
echo ""
echo "2. ${YELLOW}Serial monitor${NC} (watch output without flashing):"
echo "   python3 -m platformio device monitor"
echo "   # Or: screen /dev/ttyUSB0 115200"
echo ""
echo "3. ${YELLOW}Build + verify${NC} (check for errors):"
echo "   python3 -m platformio run -v"
echo ""
echo -e "${YELLOW}Note:${NC} Full testing requires hardware because:"
echo "  - WiFi/Network features need ESP32"
echo "  - UART/Serial2 needs hardware"
echo "  - NVS storage needs ESP32 flash"
echo ""
echo -e "${GREEN}Running compile check...${NC}"
python3 -m platformio run


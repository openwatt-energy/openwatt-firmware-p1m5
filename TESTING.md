# Testing Guide for OpenWatt P1 Reader Firmware

## Quick Start

```bash
# Run all unit tests
cd openwatt-p1-reader
pio test -e test

# Compile firmware
pio run

# Flash to device
pio run -t upload

# Monitor serial output
./scripts/monitor.sh /dev/ttyUSB0
```

## 1. Unit Tests

### Run All Unit Tests

```bash
pio test -e test
```

This will:
- Test P1 telegram parsing logic
- Test MQTT password generation
- Validate CRC16 checksum calculation
- Test edge cases and error handling

### Test Individual Components

```bash
# Test P1 parser only
pio test -e test -f "*test_p1_parser*"

# Test MQTT auth only
pio test -e test -f "*test_mqtt_auth*"
```

### Expected Output

```
test/unit/test_p1_parser.cpp:94:test_parse_valid_telegram    [PASSED]
test/unit/test_p1_parser.cpp:105:test_parse_invalid_telegram_no_crc [PASSED]
...
-----------------------
Tests passed: 5/5
```

## 2. P1 Telegram Simulator

### Setup

The simulator requires Python 3 and pyserial:

```bash
pip install pyserial
```

### Basic Usage

```bash
# Simulate continuous meter output (every 10 seconds)
python test/simulator/p1_simulator.py --port /dev/ttyUSB1 --interval 10

# Send specific number of telegrams
python test/simulator/p1_simulator.py --port /dev/ttyUSB1 --count 5

# Send test cases (valid, invalid, edge cases)
python test/simulator/p1_simulator.py --port /dev/ttyUSB1 --test-cases

# Fixed values (no variation)
python test/simulator/p1_simulator.py --port /dev/ttyUSB1 --no-vary
```

### Testing P1 Reader

1. **Connect hardware:**
   - Connect P1 meter to ESP32 Serial2 (pins 16/17)
   - Or use USB-to-serial adapter connected to `/dev/ttyUSB1`

2. **Flash firmware:**
   ```bash
   pio run -t upload
   ```

3. **Start simulator:**
   ```bash
   python test/simulator/p1_simulator.py --port /dev/ttyUSB1
   ```

4. **Monitor output:**
   ```bash
   ./scripts/monitor.sh /dev/ttyUSB0
   ```

5. **Expected output:**
   ```
   [P1 Task] Starting...
   [P1 Task] Initializing Serial2...
   P1 reader task started
   Valid telegram received (512 bytes)
   ```

## 3. Debug Logging

### Set Log Level

In `src/debug_logger.cpp`, change `LOG_LEVEL` or use runtime API:

```cpp
DebugLogger::setLevel(LOG_DEBUG);  // Show all logs
DebugLogger::setLevel(LOG_INFO);   // Show INFO, WARN, ERROR
DebugLogger::setLevel(LOG_ERROR);   // Show only errors
```

### Filter by Module

```cpp
// Show only P1 reader logs
DebugLogger::setModuleFilter(MODULE_P1);

// Show only MQTT logs
DebugLogger::setModuleFilter(MODULE_MQTT);

// Show all modules
DebugLogger::setModuleFilter("");
```

### Example Output

```
[00:00:05.123] [INFO ] [P1] Telegram received (512 bytes)
[00:00:05.456] [WARN ] [MQTT] Connection failed, retrying...
[00:00:10.789] [ERROR] [OTA] Failed to download firmware
```

## 4. MQTT Testing

### Enable MQTT

1. **Edit `src/config.h`:**
   ```cpp
   #define ENABLE_MQTT 1
   ```

2. **Configure MQTT settings via Web UI:**
   - Navigate to `http://<device-ip>/settings`
   - Enter MQTT host: `mqtt.example.com`
   - Enter MQTT port: `8883`
   - Enter MQTT topic: `p1m5/<device-id>`

### Test MQTT Connection

1. **Monitor serial output:**
   ```bash
   ./scripts/monitor.sh /dev/ttyUSB0
   ```

2. **Expected logs:**
   ```
   [INFO ] [MQTT] MQTT configured:
   [INFO ] [MQTT]   Host: mqtt.example.com:8883
   [INFO ] [MQTT]   TLS: Yes
   [INFO ] [MQTT] Attempting MQTT connection...
   [INFO ] [MQTT] MQTT connected as P1846680
   ```

3. **Verify on MQTT broker:**
   ```bash
   # Using mosquitto_sub (if you have access)
   mosquitto_sub -h mqtt.example.com -p 8883 \
     -u <device-id> -P <password> \
     -t "p1m5/<device-id>/#" -v
   ```

### Test Password Generation

```bash
# Run unit test
pio test -e test -f "*test_mqtt_auth*"

# Or test manually in Python (matching firmware logic)
python3 << EOF
import hashlib
import base64

device_id = "P1846680"
secret_key = "test_secret_key"
combined = secret_key + device_id
hash_bytes = hashlib.sha256(combined.encode()).digest()
hash_10 = hash_bytes[:10]
password = base64.b64encode(hash_10).decode()
print(f"Password: {password}")
EOF
```

## 5. OTA Update Testing

### Enable OTA

1. **Edit `src/config.h`:**
   ```cpp
   #define ENABLE_OTA 1
   ```

2. **Flash firmware:**
   ```bash
   pio run -t upload
   ```

### Test OTA Check (Manual)

1. **Via API:**
   ```bash
   curl -X PATCH http://<device-ip>/api/system/check-update
   ```

2. **Monitor serial output:**
   ```bash
   ./scripts/monitor.sh /dev/ttyUSB0
   ```

3. **Expected logs:**
   ```
   [INFO ] [OTA] Checking for firmware update...
   [INFO ] [OTA]   Serial: P1846680
   [INFO ] [OTA]   Version: v1.0.0-openwatt
   [INFO ] [OTA] Firmware is up to date
   ```

### Test OTA Update (With Test Server)

1. **Set custom firmware URL (for testing):**
   ```cpp
   OTAClient::setFirmwareURL("https://your-test-server.com/firmware");
   ```

2. **Trigger update:**
   ```bash
   curl -X PATCH http://<device-ip>/api/system/check-update
   ```

3. **Monitor progress:**
   ```
   [INFO ] [OTA] Update available:
   [INFO ] [OTA]   Name: p1m5_v1.0.1.bin
   [INFO ] [OTA] Downloading firmware update...
   [INFO ] [OTA] Progress: 10% (10240/102400 bytes)
   [INFO ] [OTA] Progress: 20% (20480/102400 bytes)
   ...
   [INFO ] [OTA] Firmware update successful! Rebooting...
   ```

### Automatic OTA Check

OTA checks automatically every 24 hours. To test immediately:

1. **Modify check interval** in `src/ota_update.cpp`:
   ```cpp
   static const unsigned long CHECK_INTERVAL_MS = 60 * 1000;  // 1 minute for testing
   ```

2. **Reboot device** and wait for automatic check

## 6. Web UI Testing

### Access Web UI

1. **Find device IP:**
   ```bash
   # Check serial output
   ./scripts/monitor.sh /dev/ttyUSB0
   # Look for: "Web UI: http://192.168.x.x"
   ```

2. **Open in browser:**
   ```
   http://<device-ip>/
   ```

### Test API Endpoints

```bash
# Get device config
curl http://<device-ip>/api/config

# Get device state
curl http://<device-ip>/api/state

# Get system info
curl http://<device-ip>/api/system

# Scan WiFi networks
curl http://<device-ip>/api/wifi/scan

# Update WiFi config
curl -X PATCH http://<device-ip>/api/config/wifi \
  -H "Content-Type: application/json" \
  -d '{"wifi":{"ssid":"YourSSID","password":"YourPassword"}}'

# Update MQTT config
curl -X PATCH http://<device-ip>/api/config/mqtt \
  -H "Content-Type: application/json" \
  -d '{"mqtt":{"host":"mqtt.example.com","port":8883,"topic":"p1m5/P1846680"}}'

# Check for OTA update
curl -X PATCH http://<device-ip>/api/system/check-update

# Reboot device
curl -X PATCH http://<device-ip>/api/system/reboot
```

## 7. Integration Testing

### Full System Test

1. **Flash firmware with all features enabled:**
   ```cpp
   // src/config.h
   #define ENABLE_P1_READER 1
   #define ENABLE_MQTT 1
   #define ENABLE_OTA 1
   ```

2. **Configure device:**
   - Connect to WiFi via Web UI
   - Configure MQTT settings
   - Connect P1 meter or simulator

3. **Verify all systems:**
   ```bash
   # Check serial output
   ./scripts/monitor.sh /dev/ttyUSB0
   
   # Check Web UI
   curl http://<device-ip>/api/state
   # Should show: wifi_connected: true, meter_connected: true, cloud_connected: true
   
   # Verify MQTT messages
   mosquitto_sub -h mqtt.example.com -p 8883 -t "p1m5/#" -v
   ```

## 8. Wokwi Simulation

### Setup

1. **Open Wokwi:** https://wokwi.com
2. **Create new ESP32 project**
3. **Import configuration:**
   - Copy `wokwi/diagram.json` content
   - Paste into Wokwi diagram editor

### Test UI/API

1. **Upload firmware code**
2. **Access simulated device IP**
3. **Test Web UI and API endpoints**
4. **Note:** Serial2/P1 reader won't work in simulation (use Python simulator for that)

## 9. Troubleshooting

### Unit Tests Fail

```bash
# Check test output for details
pio test -e test -v

# Rebuild test environment
pio test -e test --clean
```

### P1 Reader Not Working

1. **Check Serial2 pins:**
   - RX: GPIO 16
   - TX: GPIO 17
   - Baud: 115200

2. **Enable P1 reader:**
   ```cpp
   #define ENABLE_P1_READER 1
   ```

3. **Check serial output for errors:**
   ```bash
   ./scripts/monitor.sh /dev/ttyUSB0
   ```

### MQTT Connection Fails

1. **Check WiFi connection:**
   ```bash
   curl http://<device-ip>/api/state
   ```

2. **Verify MQTT credentials:**
   - Device ID matches username
   - Password generated correctly (check unit test)

3. **Check TLS:**
   - Port should be 8883 for TLS
   - Certificate validation may be disabled (setInsecure())

### OTA Update Fails

1. **Check WiFi connection**
2. **Verify API endpoint:**
   ```bash
   curl -I https://api.example.com/p1m5/firmware \
     -H "xenn-serial: P1846680" \
     -H "xenn-version: v1.0.0-openwatt" \
     -H "xenn-target: esp32"
   ```

3. **Check available flash space:**
   ```cpp
   Serial.println("Free sketch space: " + String(ESP.getFreeSketchSpace()));
   ```

## 10. Test Checklist

- [ ] Unit tests pass (`pio test -e test`)
- [ ] Firmware compiles without errors (`pio run`)
- [ ] P1 simulator sends telegrams correctly
- [ ] P1 reader parses telegrams and validates CRC
- [ ] MQTT connects with TLS and authentication
- [ ] MQTT publishes P1 data correctly
- [ ] OTA check works (HEAD request)
- [ ] OTA download works (GET request)
- [ ] Web UI loads and displays data
- [ ] API endpoints respond correctly
- [ ] WiFi connects and maintains connection
- [ ] Serial2 initialization doesn't crash
- [ ] Debug logging shows appropriate information

## Next Steps

1. **Run unit tests** to verify logic
2. **Test P1 simulator** with hardware
3. **Enable features incrementally** (MQTT, OTA, P1 reader)
4. **Monitor serial output** for errors
5. **Test Web UI** and API endpoints
6. **Verify MQTT** publishes data
7. **Test OTA** update flow

For questions or issues, check serial output with `./scripts/monitor.sh` and review debug logs.

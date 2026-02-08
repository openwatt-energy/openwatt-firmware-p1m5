# Quick Start Testing Guide

## 1. Run Unit Tests (No Hardware Needed)

```bash
cd openwatt-p1-reader
pio test -e test
```

**Expected:** All tests pass ✅

## 2. Compile and Flash Firmware

```bash
# Compile
pio run

# Flash to device (adjust port if needed)
pio run -t upload -e m5stack-core-esp32

# Monitor serial output
./scripts/monitor.sh /dev/ttyUSB0
```

## 3. Test Web UI

1. **Find device IP** in serial output:
   ```
   Web UI: http://192.168.4.1
   ```

2. **Open in browser:**
   ```
   http://192.168.4.1
   ```

3. **Test API:**
   ```bash
   curl http://192.168.4.1/api/system
   ```

## 4. Test P1 Reader (With Simulator)

### Option A: Use Python Simulator

```bash
# Install dependencies
pip install pyserial

# Run simulator (connect USB-to-serial to /dev/ttyUSB1)
python test/simulator/p1_simulator.py --port /dev/ttyUSB1 --interval 5

# In another terminal, monitor device
./scripts/monitor.sh /dev/ttyUSB0
```

**Expected:** Device receives and parses telegrams ✅

### Option B: Use Real P1 Meter

1. Connect P1 meter to ESP32 Serial2 (GPIO 16/17)
2. Enable P1 reader in `src/config.h`:
   ```cpp
   #define ENABLE_P1_READER 1
   ```
3. Reflash and monitor

## 5. Test MQTT (Optional)

1. **Enable MQTT** in `src/config.h`:
   ```cpp
   #define ENABLE_MQTT 1
   ```

2. **Configure secret key** (get from your secrets):
   ```cpp
   // Option 1: Add to config.h temporarily
   #define MQTT_SECRET_KEY "your-secret-key-here"
   
   // Option 2: Store in NVS via API (recommended)
   ```

3. **Configure via Web UI:**
   - Go to Settings
   - Enter MQTT host: `mqtt.example.com`
   - Enter MQTT port: `8883`
   - Enter MQTT topic: `p1m5/<your-device-id>`

4. **Monitor connection:**
   ```bash
   ./scripts/monitor.sh /dev/ttyUSB0
   # Look for: "MQTT connected as P1846680"
   ```

## 6. Test OTA Updates (Optional)

1. **Enable OTA** in `src/config.h`:
   ```cpp
   #define ENABLE_OTA 1
   ```

2. **Trigger check via API:**
   ```bash
   curl -X PATCH http://<device-ip>/api/system/check-update
   ```

3. **Monitor serial output** for update status

## Testing Order (Recommended)

1. ✅ **Unit tests** - Verify logic works
2. ✅ **Web UI** - Verify basic functionality
3. ✅ **P1 Simulator** - Test P1 reader without hardware
4. ✅ **MQTT** - Test cloud connectivity
5. ✅ **OTA** - Test firmware updates
6. ✅ **Real Hardware** - Final validation

## Common Issues

### Unit Tests Fail
- Check PlatformIO is installed: `pio --version`
- Rebuild: `pio test -e test --clean`

### Can't Flash Device
- Check USB connection: `ls /dev/ttyUSB*`
- Try different port: `pio run -t upload --upload-port /dev/ttyUSB0`

### P1 Reader Not Working
- Check `ENABLE_P1_READER` is set to `1`
- Verify Serial2 pins (16/17) are correct
- Check baud rate matches meter (usually 115200)

### MQTT Connection Fails
- Verify WiFi is connected
- Check secret key is configured
- Verify device ID matches username
- Check port is 8883 for TLS

## Next Steps

See `TESTING.md` for detailed testing procedures.

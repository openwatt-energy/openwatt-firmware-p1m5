# Testing Options for OpenWatt P1 Reader

## Local Testing (No Hardware)

### 1. Compile Verification
```bash
python3 -m platformio run
```
Verifies code compiles without errors.

### 2. Wokwi Simulator
- **Online**: https://wokwi.com - Create ESP32 project, paste code
- **VS Code**: Install "Wokwi" extension for local simulation
- **Limitations**: Timing differs, watchdog may behave differently, pin conflicts might not be detected

### 3. Serial Monitor (Watch Output)
```bash
python3 -m platformio device monitor
# Or: screen /dev/ttyUSB0 115200
```
Connect to already-flashed device to see Serial output without reflashing.

## Hardware Testing

### Quick Flash & Test
```bash
./build-and-flash.sh remote  # Flash via piflash
# Or locally:
./build-and-flash.sh         # Auto-detects port
```

### Debug Workflow
1. Make changes
2. `pio run` - verify compilation
3. Flash once
4. Use serial monitor to debug without reflashing
5. Reflash only when needed

## Limitations of Emulators

**What emulators CAN catch:**
- Logic bugs
- String/buffer issues
- Basic state machine problems
- Compilation errors

**What emulators CANNOT catch:**
- Watchdog timing issues (your current problem)
- Pin conflicts (Serial2 crash)
- Stack overflows
- Real-time RTOS behavior
- Hardware initialization failures

## Recommendation

For your current watchdog/Serial2 issues:
- **Use hardware** - these are hardware-specific problems
- **Add debug logging** - helps identify where crashes occur
- **Test incrementally** - disable P1 reader, test, then enable


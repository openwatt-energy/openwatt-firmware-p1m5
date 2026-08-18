# LED Indicator

The dongle has a single RGB LED (SK6812) on the front face. Its colour and
blink pattern tell you the device status at a glance.

## Quick reference

| Colour | Pattern | Meaning |
|--------|---------|---------|
| 🔴 Red | Solid | Booting — device is starting up |
| 🔴 Red | Fast blink (~5 Hz) | Unknown error — contact support |
| 🟠 Orange | Solid | Firmware (OTA) update in progress — the device reboots automatically when done |
| 🔵 Blue | Solid | AP mode ready — WiFi not configured; the device creates its own WiFi network for setup |
| 🔵 Blue | Slow blink (~1 Hz) | AP mode — no WiFi access point found |
| 🔵 Blue | Slow blink (~1 Hz) | AP mode — no P1 meter data received |
| 🟢 Green | Solid | All good — WiFi, meter and cloud (MQTT) connected |
| 🟢 Green | Slow blink (~1 Hz) | WiFi OK, but no P1 meter data received |
| 🟢 Green | Double blink | WiFi OK, but no cloud (MQTT) connection |
| 🟢 Green | Triple blink | WiFi OK, but no P1 meter data **and** no cloud connection |

## Blink patterns

| Pattern | Timing |
|---------|--------|
| Solid | LED on continuously |
| Slow (~1 Hz) | 500 ms on, 500 ms off |
| Fast (~5 Hz) | 100 ms on, 100 ms off |
| Double | Two quick blinks, then a pause (1.5 s cycle) |
| Triple | Three quick blinks, then a pause (2 s cycle) |

## Colour codes

| Colour | RGB |
|--------|-----|
| Red | `255, 0, 0` |
| Orange | `255, 165, 0` |
| Green | `0, 255, 0` |
| Blue | `0, 0, 255` |

> Source of truth: `src/led_handler.h` and `src/led_handler.cpp`.
> The LED is wired to GPIO 27.

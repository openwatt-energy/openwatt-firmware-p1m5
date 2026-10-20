# OpenWatt P1 Dongle — User Manual

The **OpenWatt P1 dongle** reads your smart meter through its **P1 port** and
streams the data to the OpenWatt platform over your home WiFi.

> ⚠️ Real-time meter data is **not stored by the meter**. If it is not collected
> at the moment it is emitted, it is lost. The dongle captures every message
> (every ~5 seconds) and forwards it continuously.

## What you need

- OpenWatt P1 dongle (ESP32 / M5Stack Atom)
- P1 cable (RJ11)
- USB-C power adapter
- A 2.4 GHz WiFi network with internet access
- A smart meter with an **activated P1 port**

## Installation

### 1. Check the P1 port is activated

On most Belgian smart meters the P1 port is enabled by default, but on some it
must be activated by your grid operator. Open the meter's protective cover and
confirm the P1 (RJ11) port is present and active.

### 2. Connect the cable to the dongle

Plug the RJ11 connector of the P1 cable into the dongle.

### 3. Connect the dongle to the meter

Plug the other end of the P1 cable into the **P1 port** of the smart meter.
See [Hardware](HARDWARE.md) for the connection diagram.

### 4. Power on

Connect the dongle to a USB-C power source. On first boot the LED turns
**red** (booting) and the dongle starts an access point for setup.

### 5. Connect to your WiFi

1. On your phone or computer, connect to the WiFi network named
   `OpenWatt-P1xxxx`.
2. Open `http://192.168.4.1` in a browser.
3. Go to **Settings**, select/enter your WiFi SSID and password, and **Save**.
4. The dongle restarts and connects to your home WiFi.

### 6. First connection to OpenWatt

Once the LED is **solid green**, the dongle is streaming data. Log in to
[openwatt.be](https://www.openwatt.be) to see your live data.

## LED status

The LED on the dongle reports its status. See
[LED Indicator](LED_INDICATOR.md) for the full table.

## Video guide

[Watch the setup video](https://www.youtube.com/watch?v=WKSMKdusNcw) — Xenn P1
dongle configuration.

## Factory reset / re-pairing

To reset all WiFi and configuration settings:

- **Web UI:** Settings → **Factory Reset**, or
- **API:** `PATCH http://<device-ip>/api/system/factory_reset`

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| Red fast blink | Unknown error | Contact support@openwatt.be |
| Blue slow blink | No WiFi AP found, or no meter data in AP mode | Check WiFi availability / check P1 port |
| Green slow blink | No P1 meter data | Check meter display / P1 port is open |
| Green double blink | No cloud connection | Check outbound MQTT on port 8883 (firewall) |
| Green triple blink | No meter data and no cloud | Check P1 port and MQTT 8883 |
| Can't open `192.168.4.1` | Not connected to the dongle's AP | Connect to `OpenWatt-P1xxxx` first |

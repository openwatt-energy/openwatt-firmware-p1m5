# OpenWatt P1M5 Documentation

User-facing and hardware documentation for the OpenWatt P1 dongle.

| Document | Description |
|----------|-------------|
| [User Manual](USER_MANUAL.md) | What the dongle does, how to install it, and how to get started |
| [LED Indicator](LED_INDICATOR.md) | Meaning of every LED colour and blink pattern |
| [Hardware](HARDWARE.md) | Dongle hardware, pinout, meter connection, buttons and reset |
| [OTA Server](OTA_SERVER.md) | Firmware update protocol and server requirements |

## Images & media

Diagrams and photo placeholders live in [`images/`](images/). See
[`images/README.md`](images/README.md) for what to drop in.

## Soliseco – AWS IoT Integration

Technical documentation for the integration of the OpenWatt P1M5
dongle with the Soliseco platform.

| Document | Description |
|----------|-------------|
| [AWS IoT Integration](SOLISECO_AWS_IOT.md) | MQTT configuration, AWS IoT connectivity, TLS authentication and topics |
| [Soliseco Tests](TESTS_SOLISECO.md) | Connection tests, MQTT publishing results and remaining validations |
| [Device Provisioning](PROVISIONING_SOLISECO.md) | Device identification, AWS/Django provisioning and certificate installation |

### Current development status

- AWS IoT Core connection successfully tested using Python.
- MQTT publishing confirmed by the AWS IoT broker.
- Official test certificates received for device P1TEST001.
- Firmware MQTT integration implemented but not yet validated on physical hardware.
- Separate certificate installation required for production devices.
- Final ESP32 hardware selection pending discussion with Emilien.
- Final device identification strategy pending validation.
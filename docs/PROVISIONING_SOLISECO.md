# Soliseco – Device Provisioning

## 1. Overview

This document describes the current device provisioning
approach for the OpenWatt P1M5 dongle.

The objective is to assign a unique identity to each device
and establish secure communication with AWS IoT Core.

**Status:** Provisioning integration is not yet validated
on physical ESP32 hardware.

## 2. Device Identification

The firmware currently generates a device ID from part
of the ESP32 MAC address.

Example:

`P1A1B2C3`

A temporary firmware option allows the use of a fixed
test identifier:

`P1TEST001`

The final device identification strategy must be
validated with Emilien.

## 3. AWS IoT Provisioning

The project contains an AWS IoT provisioning component
in the following directory:

`aws-iot-provisioning/`

The intended provisioning process includes:

1. Registering the device in AWS IoT.
2. Creating or associating device credentials.
3. Registering the device in the Soliseco Django backend.
4. Installing the credentials on the physical dongle.

The complete process has not yet been validated
for the P1M5 hardware.

## 4. Certificate Management

AWS IoT authentication requires:

- Amazon Root CA
- Device certificate
- Device private key

Official certificates were received for the
test device `P1TEST001`.

These certificates were successfully used during
Python MQTT connection and publication tests.

### Security Requirements

- Never commit private keys to Git.
- Never include private keys in documentation.
- Do not expose certificates or keys in screenshots.
- Store device credentials securely.
- Restrict access to private keys.

## 5. Firmware Integration

The current firmware uses certificate constants
defined in `mqtt_certificates.h`.

The target approach is to install device credentials
separately using a provisioning CLI, following
the method used for Metop devices.

The exact CLI procedure and secure storage mechanism
still need to be confirmed.

The firmware must eventually load the installed
credentials instead of relying on device-specific
credentials compiled into the firmware.

## 6. MQTT Configuration

Current staging configuration:

| Parameter | Value |
|-----------|-------|
| Environment | STAGING |
| Test device | P1TEST001 |
| MQTT port | 8883 |
| Authentication | X.509 |
| Readings topic | P1M5-STAGING/P1TEST001/data/readings |
| Config topic | P1M5-STAGING/P1TEST001/sys/config |

The final topic naming convention remains subject
to confirmation by the Soliseco team.

## 7. Open Questions

### For Emilien

- Which ESP32 model should be selected?
- Should device IDs be derived from MAC addresses
  or assigned during provisioning?
- Can the Metop provisioning CLI be reused?
- Where should certificates and private keys
  be stored on the ESP32?
- How should the firmware retrieve installed credentials?
- Which PlatformIO environment should be used
  for the selected hardware?

### For Soliseco

- What is the final MQTT topic structure?
- How will device registration be coordinated
  between AWS IoT and Django?
- How will backend ingestion be verified?

## 8. Remaining Work

- Confirm the provisioning procedure with Emilien.
- Adapt the firmware to load provisioned credentials.
- Validate secure credential storage.
- Confirm device ID consistency across systems.
- Compile the firmware for the selected ESP32.
- Test provisioning on physical hardware.
- Verify AWS IoT connectivity from the dongle.

## 9. Current Status

AWS IoT connectivity and MQTT publishing have
been validated using Python and the official
P1TEST001 test credentials.

Provisioning and credential installation on
physical P1M5 hardware remain to be implemented
and tested.
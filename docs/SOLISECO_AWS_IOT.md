# Soliseco – AWS IoT Integration

## 1. Overview

This document describes the integration of the OpenWatt P1M5
firmware with the Soliseco AWS IoT platform.

The objective is to transmit electricity meter readings
securely from an ESP32-based dongle to AWS IoT Core.

## 2. System Architecture

The communication architecture is:

Smart Meter → P1 Dongle (ESP32) → Wi-Fi → AWS IoT Core → Soliseco Backend

The firmware reads electricity meter data, converts it into
JSON messages and publishes them using MQTT over TLS.

## 3. AWS IoT Configuration

| Parameter | Value |
|-----------|-------|
| MQTT broker | a1rskt6dx2e2f2-ats.iot.eu-central-1.amazonaws.com |
| MQTT port | 8883 |
| Protocol | MQTT over TLS |
| Authentication | X.509 client certificates |
| Test device ID | P1TEST001 |
| Environment | STAGING |

No MQTT username or password is required for X.509 authentication.

## 4. MQTT Topics

The following topics are currently configured for testing:

### Meter readings

`P1M5-STAGING/P1TEST001/data/readings`

### Device configuration

`P1M5-STAGING/P1TEST001/sys/config`

The final topic structure must still be validated with
the Soliseco backend team.

## 5. MQTT Payload

The firmware generates JSON messages containing:

- Device identification
- Meter identification
- Electricity consumption
- Electricity production
- Power measurements
- Current and voltage measurements
- Message identifiers and timestamps

Measurement fields use OBIS identifiers where applicable.

Some optional fields depend on the meter and its capabilities.

## 6. TLS Authentication

AWS IoT Core uses mutual TLS authentication with:

- Amazon Root CA
- Device certificate
- Device private key

Official test certificates were provided for P1TEST001.

The Python MQTT test successfully connected and published
a simulated JSON message using these certificates.

### Certificate Security

Private keys must never be committed to GitHub.

The intended deployment approach is to install device
certificates separately after AWS/Django provisioning,
following the method used for Metop devices.

The firmware currently references certificate constants
defined in mqtt_certificates.h.

The separate certificate installation mechanism has
not yet been implemented or validated in the firmware.

## 7. Device Identification

The firmware currently generates a device identifier
using part of the ESP32 MAC address.

A temporary staging option allows the firmware to
use the fixed test identifier P1TEST001.

The final identification strategy must be discussed
with Emilien.

## 8. Tests Completed

### Python AWS IoT Test

Status: SUCCESS

- TLS connection established.
- MQTT connection accepted.
- JSON message published to the readings topic.
- MQTT PUBACK received with a successful result.

The published values were simulated test data,
not measurements from a physical electricity meter.

### Remaining Issues

The Python client also experienced unexpected MQTT
disconnections followed by automatic reconnections.

The cause has not yet been identified.

## 9. Remaining Work

- Implement secure certificate installation on the ESP32.
- Validate the final device identification strategy.
- Confirm the final MQTT topic structure.
- Confirm backend ingestion with Soliseco.
- Compile and validate the Soliseco firmware build.
- Test the firmware on physical ESP32 hardware.
- Verify real meter readings and MQTT transmission.

## 10. Current Status

AWS IoT connectivity and publishing have been validated
from a computer using Python.

The complete firmware-to-cloud communication chain
has not yet been validated on physical hardware.
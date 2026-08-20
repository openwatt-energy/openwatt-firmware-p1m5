# OpenWatt P1M5 MQTT API

The OpenWatt P1M5 dongle supports bidirectional communication via MQTT. This allows the OpenWatt control center to remotely configure the dongle and use it as a local network proxy (HTTP, Modbus, TCP Scanner).

## Done / to do
### Done
 - Tesla Wall readings
 - SMA Sunny Boy monitor

### To do
 - SMA Sunny Boy throttling
 - SMA Sunny Boy monitor over HTTP
 - SolisEco monitor
 - SolisEco control
 - Shelly models
 - HTTP proxy over MQTT


## Topics
*   **Command Topic:** `<topic_prefix><device_id>/cmd` (e.g., `P1M5/P1850D1C/cmd`)
*   **Response Topic:** `<topic_prefix><device_id>/cmd_response` (e.g., `P1M5/P1850D1C/cmd_response`)

---

## 1. Proxy Commands

Proxy commands are used to bridge the OpenWatt cloud with other devices on the user's local network. They use the `"action"` key.

### HTTP Request Proxy
Allows the dongle to perform REST API calls or emulate browser requests to local devices (like EV chargers or inverters).

**Request:**
```json
{
  "action": "http_request",
  "req_id": "unique-id-123",
  "method": "POST",
  "url": "http://192.168.1.50/api/v1/control",
  "headers": {
    "User-Agent": "OpenWatt-Proxy/1.1",
    "Content-Type": "application/json"
  },
  "body": "{\"state\": \"on\"}",
  "timeout_ms": 5000
}
```

**Response:**
```json
{
  "action": "http_response",
  "req_id": "unique-id-123",
  "status_code": 200,
  "body": "{\"success\": true}"
}
```

### Modbus TCP Proxy
Reads Modbus registers from devices on the local network (like solar inverters) using native TCP without heavy libraries.

**Request:**
```json
{
  "action": "modbus_read",
  "req_id": "unique-id-124",
  "ip": "192.168.1.60",
  "port": 502,
  "unit_id": 1,
  "function": 3,
  "address": 40001,
  "count": 10
}
```

**Response:**
```json
{
  "action": "modbus_response",
  "req_id": "unique-id-124",
  "data": [2300, 500, 0, 0, 1, 0, 0, 0, 0, 0]
}
```

### Local Network Scanner
Sweeps a range of IP addresses for specific open ports to automatically discover supported assets.

**Request:**
```json
{
  "action": "scan_ports",
  "req_id": "unique-id-125",
  "base_ip": "192.168.1.",
  "start_ip": 1,
  "end_ip": 254,
  "ports": [80, 502, 8080]
}
```

**Response:**
```json
{
  "action": "scan_result",
  "req_id": "unique-id-125",
  "found": [
    {"ip": "192.168.1.50", "port": 80},
    {"ip": "192.168.1.60", "port": 502}
  ]
}
```

---

## 2. System Commands

System commands control the dongle itself. They use the `"cmd"` key.

### Reboot
Restarts the ESP32.
**Request:**
```json
{
  "cmd": "reboot"
}
```

### Dump Configuration (NVS)
Retrieves the current diagnostic and non-volatile storage configuration of the dongle.
**Request:**
```json
{
  "cmd": "nvs_dump"
}
```
**Response (Published to `<prefix><device_id>/nvs_dump`):**
```json
{
  "device_id": "P1850D1C",
  "wifi_ssid": "HomeNetwork",
  "wifi_password_length": 12,
  "reboot_count": 45,
  "mqtt_host": "mqtt.example.com",
  "mqtt_topic": "P1M5/"
}
```

### Check OTA
Manually triggers a check for an Over-The-Air firmware update.
**Request:**
```json
{
  "cmd": "check_ota"
}
```

### Enable/Disable Raw Debug
Publishes raw P1 telegrams to `<prefix><device_id>/raw` for debugging meter compatibility issues.
**Enable Request:**
```json
{
  "cmd": "enable_raw_debug",
  "duration": 300
}
```
*(duration is in seconds)*

**Disable Request:**
```json
{
  "cmd": "disable_raw_debug"
}
```


# Commands
DONGLE_ID=P1850D1C

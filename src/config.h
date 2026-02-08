#ifndef CONFIG_H
#define CONFIG_H

// Feature flags — native Espressif (PlatformIO) build is the main firmware
#define ENABLE_P1_READER 1  // DSMR P1 reader enabled
#define ENABLE_MQTT 1       // MQTT client enabled for mqtt.example.com
#define ENABLE_OTA 1        // OTA updates enabled

// Firmware version (bump on release: minor = features, patch = fixes)
#define FIRMWARE_VERSION "v1.1.0"
#define DEVICE_NAME_PREFIX "OpenWatt-P1"
#define SALT_STRING "CHANGE_ME_SALT"

// MQTT Configuration for mqtt.example.com
#define MQTT_BROKER_HOST "mqtt.example.com"
#define MQTT_BROKER_PORT 8883  // TLS port
#define MQTT_BROKER_PORT_PLAIN 1883  // Plain port (for testing)
#define MQTT_DEFAULT_TOPIC "P1M5/"
// Production secret key for password generation (matches setup_auth.py)

#endif

#ifndef CONFIG_H
#define CONFIG_H

// Feature flags — native Espressif (PlatformIO) build is the main firmware
#define ENABLE_P1_READER 1  // DSMR P1 reader enabled
#define ENABLE_MQTT 1       // MQTT client enabled (broker: mqtt.openwatt.eu)
#define ENABLE_OTA 1        // OTA updates enabled

// Include customer config first (if defined) for version suffix and settings
#ifdef CUSTOMER_CONFIG_H
  #include "customer_config.h"
#endif

// Firmware version (bump on release: minor = features, patch = fixes)
#define FIRMWARE_VERSION_BASE "v1.0.47"

#ifdef FIRMWARE_VERSION_SUFFIX
  #define FIRMWARE_VERSION FIRMWARE_VERSION_BASE FIRMWARE_VERSION_SUFFIX
#else
  #define FIRMWARE_VERSION FIRMWARE_VERSION_BASE "-ow"
#endif

// Customer settings - use defaults if not using customer config
#ifndef CUSTOMER_NAME
  // Default OpenWatt values
  #define CUSTOMER_NAME "openwatt"
  #define CUSTOMER_DISPLAY_NAME "OpenWatt"
  #define CUSTOMER_FINGERPRINT_DEFAULT ""
  #define AP_SSID_PREFIX "OpenWatt"

  // Shared secret used to derive MQTT passwords:
  //   password = base64(SHA256(SALT_STRING + deviceId)[:10])
  // Must match the salt configured on the MQTT broker (mqtt.openwatt.eu).
  // Inject the real value at build time (e.g. -DSALT_STRING='"<secret>"');
  // never commit the real salt.
  #ifndef SALT_STRING
  #define SALT_STRING "CHANGE_ME_SALT"
  #endif

  // MQTT Configuration
  #define MQTT_BROKER_HOST "mqtt.openwatt.eu"
  #define MQTT_BROKER_PORT 8883  // TLS port
  #define MQTT_BROKER_PORT_PLAIN 1883  // Plain port (for testing)
  #define MQTT_DEFAULT_TOPIC "P1M5/"
  #define MQTT_PUBLISH_INTERVAL_MS 5000

  // Feature defaults
  #define JSON_API_ENABLED 1
  #define MQTT_SETTINGS_UI_ENABLED 0

  // Theme defaults (OpenWatt blue)
  #define THEME_PRIMARY "#2563eb"
  #define THEME_BACKGROUND "#ffffff"
  #define THEME_TEXT "#111827"
  #define THEME_ACCENT "#1d4ed8"
#endif

// MQTT Configuration - Broker port (plain text fallback)
#define MQTT_BROKER_PORT_PLAIN 1883

// MQTT Publish interval (milliseconds) - default 5 seconds
#ifndef MQTT_PUBLISH_INTERVAL_MS
  #define MQTT_PUBLISH_INTERVAL_MS 5000
#endif

// MQTT Status publish interval (milliseconds) - default 60 seconds for firmware info
#define MQTT_STATUS_INTERVAL_MS 60000

// NVS Keys for runtime overrides
#define NVS_KEY_FINGERPRINT "customer_fingerprint"
#define NVS_KEY_MQTT_HOST "mqtt_host"
#define NVS_KEY_MQTT_INTERVAL "mqtt_interval"
#define NVS_KEY_REBOOT_COUNT "reboot_count"
#define NVS_KEY_CREOS_KEY "creos_key"

// NVS namespace
#define NVS_NAMESPACE "openwatt"

#endif

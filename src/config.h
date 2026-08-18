#ifndef CONFIG_H
#define CONFIG_H

// Feature flags — native Espressif (PlatformIO) build is the main firmware
#define ENABLE_P1_READER 1  // DSMR P1 reader enabled
#define ENABLE_MQTT 1       // MQTT client enabled for mqtt.example.com
#define ENABLE_OTA 1        // OTA updates enabled

// Include customer config first (if defined) for version suffix and settings
#ifdef CUSTOMER_CONFIG_H
  #include "customer_config.h"
#endif

// Firmware version (bump on release: minor = features, patch = fixes)
#define FIRMWARE_VERSION_BASE "v1.0.48-rc1"

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
  #define SALT_STRING "CHANGE_ME_SALT"

  // MQTT Configuration for mqtt.example.com
  #define MQTT_BROKER_HOST "mqtt.example.com"
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

// MQTT TLS certificate verification
// 1 = verify the broker's certificate against MQTT_CA_CERT (public CA).
// 0 = skip verification (self-signed / insecure - for custom builders only).
#define MQTT_TLS_VERIFY 0

// Root CA for MQTT TLS verification (Let's Encrypt ISRG Root X1).
// Swap this for your broker's CA if it uses a different public/private CA.
#define MQTT_CA_CERT \
  "-----BEGIN CERTIFICATE-----\n" \
  "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\n" \
  "-----END CERTIFICATE-----\n"

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

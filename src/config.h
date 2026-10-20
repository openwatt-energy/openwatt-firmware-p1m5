#ifndef CONFIG_H
#define CONFIG_H


// ============================================================================
// VARIANT CONFIGURATION
// ============================================================================
//
// Ce fichier est généré par scripts/build_variant.py.
//
// Il définit notamment :
//
// ENABLE_P1_READER
// ENABLE_LINKY_TIC
// ENABLE_MQTT
// ENABLE_OTA
//
// Exemple Linky France :
// ENABLE_P1_READER = 0
// ENABLE_LINKY_TIC = 1
//
// Exemple P1 :
// ENABLE_P1_READER = 1
// ENABLE_LINKY_TIC = 0
//

#include "variant_config.h"


// ============================================================================
// DEFAULT FEATURE FLAGS
// ============================================================================
//
// Ces valeurs sont uniquement utilisées si une option
// n'est pas définie par variant_config.h.
//

#ifndef ENABLE_P1_READER
#define ENABLE_P1_READER 1
#endif

#ifndef ENABLE_LINKY_TIC
#define ENABLE_LINKY_TIC 0
#endif

#ifndef ENABLE_MQTT
#define ENABLE_MQTT 1
#endif

#ifndef ENABLE_OTA
#define ENABLE_OTA 1
#endif


// ============================================================================
// CUSTOMER CONFIGURATION
// ============================================================================

#ifdef CUSTOMER_CONFIG_H
#include "customer_config.h"
#endif


// ============================================================================
// FIRMWARE VERSION
// ============================================================================

#define FIRMWARE_VERSION_BASE "v1.0.47"

#ifdef FIRMWARE_VERSION_SUFFIX

#define FIRMWARE_VERSION \
    FIRMWARE_VERSION_BASE FIRMWARE_VERSION_SUFFIX

#else

#define FIRMWARE_VERSION \
    FIRMWARE_VERSION_BASE "-ow"

#endif


// ============================================================================
// DEFAULT CUSTOMER SETTINGS
// ============================================================================

#ifndef CUSTOMER_NAME

#define CUSTOMER_NAME "openwatt"

#define CUSTOMER_DISPLAY_NAME "OpenWatt"

#define CUSTOMER_FINGERPRINT_DEFAULT ""

#define AP_SSID_PREFIX "OpenWatt"

#define SALT_STRING "CHANGE_ME_SALT"


// ----------------------------------------------------------------------------
// MQTT
// ----------------------------------------------------------------------------

#define MQTT_BROKER_HOST \
    "mqtt.example.com"

#define MQTT_BROKER_PORT 8883

#define MQTT_BROKER_PORT_PLAIN 1883

#define MQTT_DEFAULT_TOPIC \
    "P1M5/"

#define MQTT_PUBLISH_INTERVAL_MS 5000


// ----------------------------------------------------------------------------
// Features
// ----------------------------------------------------------------------------

#define JSON_API_ENABLED 1

#define MQTT_SETTINGS_UI_ENABLED 0


// ----------------------------------------------------------------------------
// Theme
// ----------------------------------------------------------------------------

#define THEME_PRIMARY \
    "#2563eb"

#define THEME_BACKGROUND \
    "#ffffff"

#define THEME_TEXT \
    "#111827"

#define THEME_ACCENT \
    "#1d4ed8"

#endif


// ============================================================================
// MQTT FALLBACK VALUES
// ============================================================================

#ifndef MQTT_BROKER_PORT_PLAIN
#define MQTT_BROKER_PORT_PLAIN 1883
#endif


#ifndef MQTT_PUBLISH_INTERVAL_MS
#define MQTT_PUBLISH_INTERVAL_MS 5000
#endif


// MQTT status publish interval
#define MQTT_STATUS_INTERVAL_MS 60000


// ============================================================================
// NVS KEYS
// ============================================================================

#define NVS_KEY_FINGERPRINT \
    "customer_fingerprint"

#define NVS_KEY_MQTT_HOST \
    "mqtt_host"

#define NVS_KEY_MQTT_INTERVAL \
    "mqtt_interval"

#define NVS_KEY_REBOOT_COUNT \
    "reboot_count"

#define NVS_KEY_CREOS_KEY \
    "creos_key"


// ============================================================================
// NVS NAMESPACE
// ============================================================================

#define NVS_NAMESPACE "openwatt"


#endif
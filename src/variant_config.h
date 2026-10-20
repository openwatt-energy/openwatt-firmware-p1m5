// Auto-generated variant configuration
// Variant: linky-france
// Description: OpenWatt firmware for French Linky meters using TIC
// Generated: Do not edit manually

#ifndef VARIANT_CONFIG_H
#define VARIANT_CONFIG_H

// Firmware extension (used in filename)
#define FW_EXT "linky-fr"

// SSID prefix for AP mode
#define SSID_PREFIX "OpenWatt-Linky"

// Feature flags
#define ALLOW_MQTT_CHANGES 1
#define ENABLE_MQTT 1
#define ENABLE_OTA 1

// Meter interfaces
#define ENABLE_P1_READER 0
#define ENABLE_LINKY_TIC 1

// Linky TIC mode
// 0 = historique
// 1 = standard
#define LINKY_TIC_STANDARD 0

// UI Theme
#define UI_THEME "switchgrid"

#endif // VARIANT_CONFIG_H

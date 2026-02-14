#include "wifi_manager.h"
#include "serial_console.h"
#include "config.h"
#include <Preferences.h>
#include <cstring>
#include <nvs.h>
#include <esp_system.h>

// WiFi credentials stored using Preferences API (more reliable than direct NVS)
#define PREFS_NAMESPACE "openwatt"
#define PREFS_KEY_WIFI_SSID "wifi_ssid"
#define PREFS_KEY_WIFI_PASS "wifi_password"

void WiFiManager::begin(Preferences& prefs, const String& deviceId) {
  SerialConsole::println("Starting WiFi...");

  // Load saved credentials
  WiFiConfig config = loadCredentials(prefs);

  // Set AP SSID with format: SolisEco-P1XXXXXX
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  char apName[32];
  sprintf(apName, "%s-P1%02X%02X%02X", AP_SSID_PREFIX, mac[3], mac[4], mac[5]);
  String apSSID = String(apName);

  // Always start AP mode first (open, no password)
  SerialConsole::println("Starting AP mode...");
  SerialConsole::println("  AP SSID: " + apSSID);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apSSID.c_str(), NULL);  // Open AP

  // Set static IP for AP
  IPAddress localIP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(localIP, gateway, subnet);

  SerialConsole::println("AP mode started");
  SerialConsole::println("  AP IP: " + WiFi.softAPIP().toString());

  // Try to connect to saved network if available
  if (config.ssid.length() > 0) {
    SerialConsole::println("Connecting to: " + config.ssid);
    WiFi.begin(config.ssid.c_str(), config.password.c_str());

    // Non-blocking - connection will be checked in loop
    SerialConsole::println("WiFi connection attempt started (non-blocking)");
  } else {
    SerialConsole::println("No saved WiFi credentials");
  }
}

void WiFiManager::connect(const String& ssid, const String& password) {
  WiFi.begin(ssid.c_str(), password.c_str());
}

void WiFiManager::scanNetworks() {
  WiFi.scanNetworks(true, true);  // async, show_hidden=false
}

bool WiFiManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String WiFiManager::getIP() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }
  return "";
}

String WiFiManager::getAPIP() {
  return WiFi.softAPIP().toString();
}

String WiFiManager::getConnectedSSID() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.SSID();
  }
  return "";
}

void WiFiManager::saveCredentials(Preferences& prefs, const String& ssid, const String& password) {
  saveCredentials(prefs, ssid.c_str(), password.c_str());
}

void WiFiManager::saveCredentials(Preferences& prefs, const char* ssid, const char* password) {
  if (!ssid) ssid = "";
  if (!password) password = "";

  SerialConsole::println("Saving WiFi credentials:");
  SerialConsole::println("  SSID: " + String(ssid));
  SerialConsole::println("  Password length: " + String(strlen(password)));

  // Use Preferences directly - more reliable than raw NVS
  prefs.putString(PREFS_KEY_WIFI_SSID, ssid);
  prefs.putString(PREFS_KEY_WIFI_PASS, password);

  // Verify save worked
  String verifySSID = prefs.getString(PREFS_KEY_WIFI_SSID, "");
  if (verifySSID == String(ssid)) {
    SerialConsole::println("SUCCESS: WiFi credentials saved!");
  } else {
    SerialConsole::println("ERROR: WiFi credentials verification failed!");
  }
}

WiFiConfig WiFiManager::loadCredentials(Preferences& prefs) {
  WiFiConfig config;

  // Try new format first (openwatt namespace)
  config.ssid = prefs.getString(PREFS_KEY_WIFI_SSID, "");
  config.password = prefs.getString(PREFS_KEY_WIFI_PASS, "");

  // If not found, try Xenn format (xenn namespace with string keys)
  if (config.ssid.length() == 0) {
    SerialConsole::println("WiFi: Trying to load from Xenn format (xenn namespace)...");
    nvs_handle_t h;
    esp_err_t err = nvs_open("xenn", NVS_READONLY, &h);
    if (err == ESP_OK) {
      SerialConsole::println("WiFi: Opened xenn namespace successfully");
      size_t len = 0;

      // Try "wifi_ssid" key first (prefixed format)
      if (nvs_get_str(h, "wifi_ssid", NULL, &len) == ESP_OK && len > 0) {
        char* buf = (char*)malloc(len);
        if (buf && nvs_get_str(h, "wifi_ssid", buf, &len) == ESP_OK) {
          config.ssid = String(buf);
          SerialConsole::println("WiFi: Recovered SSID from Xenn format (wifi_ssid)");
        }
        if (buf) free(buf);
      }

      // Fallback to "ssid" key (simple format)
      if (config.ssid.length() == 0) {
        len = 0;
        if (nvs_get_str(h, "ssid", NULL, &len) == ESP_OK && len > 0) {
          char* buf = (char*)malloc(len);
          if (buf && nvs_get_str(h, "ssid", buf, &len) == ESP_OK) {
            config.ssid = String(buf);
            SerialConsole::println("WiFi: Recovered SSID from Xenn format (ssid)");
          }
          if (buf) free(buf);
        }
      }

      // Try "wifi_password" key first (prefixed format)
      len = 0;
      if (nvs_get_str(h, "wifi_password", NULL, &len) == ESP_OK && len > 0) {
        char* buf = (char*)malloc(len);
        if (buf && nvs_get_str(h, "wifi_password", buf, &len) == ESP_OK) {
          config.password = String(buf);
          SerialConsole::println("WiFi: Recovered password from Xenn format (wifi_password)");
        }
        if (buf) free(buf);
      }

      // Fallback to "password" key (simple format)
      if (config.password.length() == 0) {
        len = 0;
        if (nvs_get_str(h, "password", NULL, &len) == ESP_OK && len > 0) {
          char* buf = (char*)malloc(len);
          if (buf && nvs_get_str(h, "password", buf, &len) == ESP_OK) {
            config.password = String(buf);
            SerialConsole::println("WiFi: Recovered password from Xenn format (password)");
          }
          if (buf) free(buf);
        }
      }

      nvs_close(h);

      // Save to new format if we recovered anything
      if (config.ssid.length() > 0) {
        saveCredentials(prefs, config.ssid, config.password);
        SerialConsole::println("WiFi: Migrated Xenn credentials to new format");
      } else {
        SerialConsole::println("WiFi: No SSID found in Xenn format");
      }
    } else {
      SerialConsole::println("WiFi: Failed to open wifi-settings namespace, err=" + String(err));
    }
  }

  return config;
}

String WiFiManager::getSavedSSID(Preferences& prefs) {
  return prefs.getString(PREFS_KEY_WIFI_SSID, "");
}

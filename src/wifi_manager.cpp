#include "wifi_manager.h"
#include "serial_console.h"
#include <Preferences.h>
#include <cstring>
#include <nvs.h>

#define AP_SSID_PREFIX "OpenWatt-P1"
// WiFi credentials stored using Preferences API (more reliable than direct NVS)
#define PREFS_NAMESPACE "openwatt"
#define PREFS_KEY_WIFI_SSID "wifi_ssid"
#define PREFS_KEY_WIFI_PASS "wifi_password"

void WiFiManager::begin(Preferences& prefs, const String& deviceId) {
  SerialConsole::println("Starting WiFi...");
  
  // Load saved credentials
  WiFiConfig config = loadCredentials(prefs);
  
  // Set AP SSID
  String apSSID = String(AP_SSID_PREFIX) + deviceId.substring(2);
  
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
  
  // If not found, try Xenn format (wifi-settings namespace with string keys)
  if (config.ssid.length() == 0) {
    nvs_handle_t h;
    if (nvs_open("wifi-settings", NVS_READONLY, &h) == ESP_OK) {
      size_t len = 0;
      
      // Try to get ssid as string
      if (nvs_get_str(h, "ssid", NULL, &len) == ESP_OK && len > 0) {
        char* buf = (char*)malloc(len);
        if (buf && nvs_get_str(h, "ssid", buf, &len) == ESP_OK) {
          config.ssid = String(buf);
          SerialConsole::println("WiFi: Recovered SSID from Xenn format");
        }
        if (buf) free(buf);
      }
      
      // Try to get password as string
      len = 0;
      if (nvs_get_str(h, "password", NULL, &len) == ESP_OK && len > 0) {
        char* buf = (char*)malloc(len);
        if (buf && nvs_get_str(h, "password", buf, &len) == ESP_OK) {
          config.password = String(buf);
          SerialConsole::println("WiFi: Recovered password from Xenn format");
        }
        if (buf) free(buf);
      }
      
      nvs_close(h);
      
      // Save to new format if we recovered anything
      if (config.ssid.length() > 0) {
        saveCredentials(prefs, config.ssid, config.password);
        SerialConsole::println("WiFi: Migrated Xenn credentials to new format");
      }
    }
  }
  
  return config;
}

String WiFiManager::getSavedSSID(Preferences& prefs) {
  return prefs.getString(PREFS_KEY_WIFI_SSID, "");
}

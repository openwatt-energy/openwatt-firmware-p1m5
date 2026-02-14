#include "wifi_manager.h"
#include "serial_console.h"
#include "config.h"
#include <Preferences.h>
#include <cstring>
#include <nvs.h>
#include <esp_system.h>
#include <esp_wifi.h>

// WiFi credentials stored using Preferences API (more reliable than direct NVS)
#define PREFS_NAMESPACE "openwatt"
#define PREFS_KEY_WIFI_SSID "wifi_ssid"
#define PREFS_KEY_WIFI_PASS "wifi_password"

// WiFi connection state
static bool wifiConnected = false;
static bool apEnabled = true;
static int connectAttempts = 0;
static unsigned long connectStartTime = 0;

void WiFiManager::begin(Preferences& prefs, const String& deviceId) {
  SerialConsole::println("Starting WiFi...");

  // Configure WiFi for maximum performance
  WiFi.setSleep(false);  // Disable light sleep for stable connection
  WiFi.setAutoReconnect(false);  // We handle reconnect ourselves
  WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);  // Connect to strongest signal

  // Load saved credentials
  WiFiConfig config = loadCredentials(prefs);

  // Set AP SSID with format: SolisEco-P1XXXXXX
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  char apName[32];
  sprintf(apName, "%s-P1%02X%02X%02X", AP_SSID_PREFIX, mac[3], mac[4], mac[5]);
  String apSSID = String(apName);

  // Check if we have credentials
  if (config.ssid.length() > 0) {
    // Start in AP+STA mode, try to connect
    SerialConsole::println("Starting AP+STA mode...");
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

    // Try to connect
    SerialConsole::println("Connecting to: " + config.ssid);
    WiFi.begin(config.ssid.c_str(), config.password.c_str());
    connectAttempts = 1;
    connectStartTime = millis();
    SerialConsole::println("WiFi connection attempt started (non-blocking)");
  } else {
    // No credentials - AP only
    SerialConsole::println("No saved WiFi credentials, starting AP only...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID.c_str(), NULL);  // Open AP

    // Set static IP for AP
    IPAddress localIP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(localIP, gateway, subnet);

    SerialConsole::println("AP mode started");
    SerialConsole::println("  AP IP: " + WiFi.softAPIP().toString());
    apEnabled = true;
    wifiConnected = false;
  }
}

void WiFiManager::connect(const String& ssid, const String& password) {
  WiFi.begin(ssid.c_str(), password.c_str());
  connectAttempts = 1;
  connectStartTime = millis();
}

void WiFiManager::reconnect(Preferences& prefs) {
  WiFiConfig config = loadCredentials(prefs);
  if (config.ssid.length() > 0) {
    SerialConsole::println("WiFi: Reconnecting to: " + config.ssid);
    WiFi.disconnect();
    delay(200);
    WiFi.begin(config.ssid.c_str(), config.password.c_str());
    connectAttempts = 1;
    connectStartTime = millis();
  }
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

      // Fallback to "wifi_pass" key
      if (config.password.length() == 0) {
        len = 0;
        if (nvs_get_str(h, "wifi_pass", NULL, &len) == ESP_OK && len > 0) {
          char* buf = (char*)malloc(len);
          if (buf && nvs_get_str(h, "wifi_pass", buf, &len) == ESP_OK) {
            config.password = String(buf);
            SerialConsole::println("WiFi: Recovered password from Xenn format (wifi_pass)");
          }
          if (buf) free(buf);
        }
      }

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

// Check WiFi status and handle connection state machine
// Call this periodically from main loop
void WiFiManager::checkStatus(Preferences& prefs) {
  wl_status_t status = WiFi.status();
  unsigned long now = millis();

  if (status == WL_CONNECTED) {
    if (!wifiConnected) {
      // Just connected!
      wifiConnected = true;
      connectAttempts = 0;
      SerialConsole::println("WiFi connected: " + WiFi.localIP().toString());
      SerialConsole::println("WiFi RSSI: " + String(WiFi.RSSI()) + " dBm");

      // Disable AP to save resources
      if (apEnabled) {
        SerialConsole::println("WiFi connected, disabling AP...");
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        apEnabled = false;
      }
    }
    // Connected - all good
    return;
  }

  // Not connected
  if (wifiConnected) {
    // Was connected, now disconnected
    wifiConnected = false;
    SerialConsole::println("WiFi disconnected, will retry...");
    // Try to reconnect once
    WiFi.disconnect();
    delay(100);
    WiFiConfig config = loadCredentials(prefs);
    if (config.ssid.length() > 0) {
      WiFi.begin(config.ssid.c_str(), config.password.c_str());
      connectAttempts = 1;
      connectStartTime = now;
    }
    return;
  }

  // Was never connected - check if we should retry
  if (connectAttempts > 0 && connectAttempts <= 3) {
    unsigned long elapsed = now - connectStartTime;
    unsigned long timeout = 15000;  // 15 second timeout per attempt

    if (elapsed > timeout) {
      // Timeout - retry with backoff
      SerialConsole::println("WiFi connection timeout, attempt " + String(connectAttempts) + "/3 failed");

      WiFiConfig config = loadCredentials(prefs);
      if (config.ssid.length() > 0 && connectAttempts < 3) {
        connectAttempts++;
        connectStartTime = now;
        unsigned long backoff = 1000 * connectAttempts;  // 1s, 2s, 3s
        SerialConsole::println("WiFi: Retrying in " + String(backoff) + "ms...");
        delay(backoff);
        WiFi.disconnect();
        delay(100);
        WiFi.begin(config.ssid.c_str(), config.password.c_str());
      } else if (connectAttempts >= 3) {
        // All attempts failed - enable AP fallback
        SerialConsole::println("WiFi: All 3 attempts failed, enabling AP fallback");
        WiFi.disconnect();
        delay(100);
        WiFi.mode(WIFI_AP_STA);

        // Get AP SSID
        uint8_t mac[6];
        esp_efuse_mac_get_default(mac);
        char apName[32];
        sprintf(apName, "%s-P1%02X%02X%02X", AP_SSID_PREFIX, mac[3], mac[4], mac[5]);
        WiFi.softAP(apName, NULL);

        IPAddress localIP(192, 168, 4, 1);
        IPAddress gateway(192, 168, 4, 1);
        IPAddress subnet(255, 255, 255, 0);
        WiFi.softAPConfig(localIP, gateway, subnet);

        SerialConsole::println("AP fallback enabled: " + WiFi.softAPIP().toString());
        apEnabled = true;
        connectAttempts = 0;  // Stop retrying until user saves new credentials
      }
    }
  }
}

// Enable AP mode (for when user needs to reconfigure)
void WiFiManager::enableAP() {
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  char apName[32];
  sprintf(apName, "%s-P1%02X%02X%02X", AP_SSID_PREFIX, mac[3], mac[4], mac[5]);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apName, NULL);

  IPAddress localIP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(localIP, gateway, subnet);

  SerialConsole::println("AP mode enabled: " + WiFi.softAPIP().toString());
  apEnabled = true;
  wifiConnected = false;
  connectAttempts = 0;
}

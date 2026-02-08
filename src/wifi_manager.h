#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <Preferences.h>
#include <Arduino.h>

struct WiFiConfig {
  String ssid;
  String password;
  bool apMode;
  String apSSID;
};

class WiFiManager {
public:
  static void begin(Preferences& prefs, const String& deviceId);
  static void connect(const String& ssid, const String& password);
  static void scanNetworks();
  static bool isConnected();
  static String getIP();
  static String getAPIP();
  static String getConnectedSSID();
  static void saveCredentials(Preferences& prefs, const String& ssid, const String& password);
  /** Prefer this when you have raw buffers (avoids String lifetime issues). */
  static void saveCredentials(Preferences& prefs, const char* ssid, const char* password);
  static WiFiConfig loadCredentials(Preferences& prefs);
  /** Saved SSID from NVS (for API display when not connected). */
  static String getSavedSSID(Preferences& prefs);
};

#endif

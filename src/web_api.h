#ifndef WEB_API_H
#define WEB_API_H

#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "p1_reader.h"

// Get fingerprint from NVS or compile-time default
String getFingerprint();

class WebAPI {
public:
  static void setup(AsyncWebServer& server, Preferences& prefs, const String& deviceId, const String& serialNumber);
  static void setLatestData(const P1Data& data);

private:
  static Preferences* prefs;
  static String deviceId;
  static String serialNumber;
  static P1Data latestData;
};

#endif

#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <Arduino.h>

class OTAUpdate {
public:
  static void begin(const String& deviceSerial, const String& currentVersion);
  static void loop();
  static bool checkUpdate();  // Manual check (can be called from API)
  
private:
  static String deviceSerial;
  static String currentVersion;
  static unsigned long lastCheckTime;
  static const unsigned long CHECK_INTERVAL_MS = 24 * 60 * 60 * 1000;  // 24 hours
};

#endif

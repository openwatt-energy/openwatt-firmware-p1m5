#include "ota_update.h"
#include "ota_client.h"
#include "debug_logger.h"
#include "led_handler.h"
#include <WiFi.h>

String OTAUpdate::deviceSerial = "";
String OTAUpdate::currentVersion = "";
unsigned long OTAUpdate::lastCheckTime = 0;

void OTAUpdate::begin(const String& deviceSerial, const String& currentVersion) {
  OTAUpdate::deviceSerial = deviceSerial;
  OTAUpdate::currentVersion = currentVersion;
  OTAUpdate::lastCheckTime = 0;  // Check on first loop
  
  LOG_INFO(MODULE_OTA, "OTA update service initialized");
  LOG_INFO(MODULE_OTA, "  Device Serial: %s", deviceSerial.c_str());
  LOG_INFO(MODULE_OTA, "  Current Version: %s", currentVersion.c_str());
}

bool OTAUpdate::checkUpdate() {
  if (WiFi.status() != WL_CONNECTED) {
    LOG_DEBUG(MODULE_OTA, "WiFi not connected, skipping OTA check");
    return false;
  }
  
  // Get MAC address
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  // Check for update
  OTAInfo info = OTAClient::checkForUpdate(deviceSerial, currentVersion, String(macStr));
  
  if (info.updateAvailable) {
    LOG_INFO(MODULE_OTA, "Update available, downloading...");
    LEDHandler::setOTAUpdate(true);  // Indicate OTA in progress
    
    bool success = OTAClient::downloadAndApply(deviceSerial, currentVersion, String(macStr));
    
    LEDHandler::setOTAUpdate(false);
    
    if (success) {
      // Device will reboot, so we won't reach here
      return true;
    } else {
      LOG_ERROR(MODULE_OTA, "OTA update failed");
      return false;
    }
  }
  
  return false;
}

void OTAUpdate::loop() {
  // Check for updates every 24 hours (or on first run)
  unsigned long now = millis();
  
  // Handle millis() overflow
  if (now < lastCheckTime) {
    lastCheckTime = 0;
  }
  
  if (now - lastCheckTime >= CHECK_INTERVAL_MS || lastCheckTime == 0) {
    lastCheckTime = now;
    checkUpdate();
  }
}

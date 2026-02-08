#include "ota_update.h"
#include "ota_client.h"
#include "debug_logger.h"
#include "led_handler.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// External WebSocket reference (defined in main.cpp)
extern AsyncWebSocket ws;

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
  LOG_INFO(MODULE_OTA, "  Check interval: 1 hour");
  
  // Trigger OTA check after a short delay (to allow WiFi to stabilize)
  // This ensures we check for updates on every reboot
  LOG_INFO(MODULE_OTA, "Scheduling initial OTA check in 30 seconds...");
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
    
    // Close all WebSocket clients to free up SSL/WiFi resources for OTA
    LOG_INFO(MODULE_OTA, "Closing WebSocket connections for OTA...");
    ws.closeAll();
    delay(100);  // Give time for connections to close
    
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
  // Check for updates every hour (or on first run after 30 second delay)
  unsigned long now = millis();
  
  // Handle millis() overflow
  if (now < lastCheckTime) {
    lastCheckTime = 0;
  }
  
  // On first boot (lastCheckTime == 0), wait 30 seconds for WiFi to stabilize
  // Then check every hour thereafter
  static const unsigned long BOOT_DELAY_MS = 30000;  // 30 seconds after boot
  
  if (lastCheckTime == 0 && now >= BOOT_DELAY_MS) {
    // First check after boot delay
    lastCheckTime = now;
    LOG_INFO(MODULE_OTA, "Performing initial OTA check after boot...");
    checkUpdate();
  } else if (lastCheckTime > 0 && now - lastCheckTime >= CHECK_INTERVAL_MS) {
    // Regular hourly check
    lastCheckTime = now;
    LOG_INFO(MODULE_OTA, "Performing scheduled OTA check...");
    checkUpdate();
  }
}

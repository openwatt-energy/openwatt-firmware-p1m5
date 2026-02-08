#include "ota_client.h"
#include "config.h"
#include "debug_logger.h"
#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>

#define OTA_FIRMWARE_URL "https://api.example.com/p1m5/firmware"
#define OTA_TIMEOUT_MS 30000  // 30 seconds

String OTAClient::firmwareURL = OTA_FIRMWARE_URL;

void OTAClient::setFirmwareURL(const String& url) {
  firmwareURL = url;
}

String OTAClient::getMacAddressString() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  return String(macStr);
}

void OTAClient::setUpdateHeaders(HTTPClient& client, const String& deviceSerial, const String& currentVersion, const String& macAddress) {
  client.addHeader("User-Agent", "Xenn-Update");
  client.addHeader("Cache-Control", "no-cache");
  client.addHeader("xenn-target", "esp32");
  client.addHeader("xenn-version", currentVersion);
  client.addHeader("xenn-serial", deviceSerial);
  client.addHeader("xenn-mac", macAddress);
}

OTAInfo OTAClient::checkForUpdate(const String& deviceSerial, const String& currentVersion, const String& macAddress) {
  OTAInfo info;
  info.updateAvailable = false;
  
  if (WiFi.status() != WL_CONNECTED) {
    LOG_WARN(MODULE_OTA, "WiFi not connected, cannot check for updates");
    return info;
  }
  
  HTTPClient http;
  http.begin(firmwareURL);
  http.setTimeout(OTA_TIMEOUT_MS);
  
  setUpdateHeaders(http, deviceSerial, currentVersion, macAddress);
  
  LOG_INFO(MODULE_OTA, "Checking for firmware update...");
  LOG_DEBUG(MODULE_OTA, "  Serial: %s", deviceSerial.c_str());
  LOG_DEBUG(MODULE_OTA, "  Version: %s", currentVersion.c_str());
  LOG_DEBUG(MODULE_OTA, "  MAC: %s", macAddress.c_str());
  
  int httpCode = http.sendRequest("HEAD");
  
  if (httpCode == 200) {
    // Update available
    info.updateAvailable = true;
    info.firmwareName = http.header("Xenn-Firmware-Name");
    info.firmwareTag = http.header("Xenn-Firmware-Tag");
    info.firmwareDate = http.header("Xenn-Firmware-Date");
    
    LOG_INFO(MODULE_OTA, "Update available:");
    LOG_INFO(MODULE_OTA, "  Name: %s", info.firmwareName.c_str());
    LOG_INFO(MODULE_OTA, "  Tag: %s", info.firmwareTag.c_str());
    LOG_INFO(MODULE_OTA, "  Date: %s", info.firmwareDate.c_str());
  } else if (httpCode == 304) {
    // Up to date
    LOG_INFO(MODULE_OTA, "Firmware is up to date");
    info.updateAvailable = false;
  } else {
    LOG_ERROR(MODULE_OTA, "Failed to check for update, HTTP code: %d", httpCode);
  }
  
  http.end();
  return info;
}

bool OTAClient::downloadAndApply(const String& deviceSerial, const String& currentVersion, const String& macAddress) {
  if (WiFi.status() != WL_CONNECTED) {
    LOG_ERROR(MODULE_OTA, "WiFi not connected, cannot download update");
    return false;
  }
  
  HTTPClient http;
  http.begin(firmwareURL);
  http.setTimeout(OTA_TIMEOUT_MS);
  
  setUpdateHeaders(http, deviceSerial, currentVersion, macAddress);
  
  LOG_INFO(MODULE_OTA, "Downloading firmware update...");
  
  int httpCode = http.GET();
  
  if (httpCode != 200) {
    LOG_ERROR(MODULE_OTA, "Failed to download firmware, HTTP code: %d", httpCode);
    http.end();
    return false;
  }
  
  // Get firmware info from headers
  String firmwareName = http.header("Xenn-Firmware-Name");
  String firmwareTag = http.header("Xenn-Firmware-Tag");
  String firmwareDate = http.header("Xenn-Firmware-Date");
  
  LOG_INFO(MODULE_OTA, "Firmware info:");
  LOG_INFO(MODULE_OTA, "  Name: %s", firmwareName.c_str());
  LOG_INFO(MODULE_OTA, "  Tag: %s", firmwareTag.c_str());
  LOG_INFO(MODULE_OTA, "  Date: %s", firmwareDate.c_str());
  
  // Get content length
  int contentLength = http.getSize();
  if (contentLength <= 0) {
    LOG_ERROR(MODULE_OTA, "Invalid content length: %d", contentLength);
    http.end();
    return false;
  }
  
  LOG_INFO(MODULE_OTA, "Content length: %d bytes", contentLength);
  
  // Check if we have enough space
  if (contentLength > UPDATE_SIZE_UNKNOWN && contentLength > (ESP.getFreeSketchSpace() - 0x1000)) {
    LOG_ERROR(MODULE_OTA, "Not enough space for update. Required: %d, Available: %d", 
              contentLength, ESP.getFreeSketchSpace());
    http.end();
    return false;
  }
  
  // Begin OTA update
  if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
    LOG_ERROR(MODULE_OTA, "Update.begin() failed: %s", Update.errorString());
    http.end();
    return false;
  }
  
  LOG_INFO(MODULE_OTA, "Starting firmware update...");
  
  // Stream download
  WiFiClient* stream = http.getStreamPtr();
  size_t written = 0;
  uint8_t buffer[1024];
  
  while (http.connected() && (written < contentLength || contentLength == -1)) {
    size_t available = stream->available();
    if (available) {
      int c = stream->readBytes(buffer, min(available, sizeof(buffer)));
      Update.write(buffer, c);
      written += c;
      
      // Log progress every 10%
      if (contentLength > 0) {
        int percent = (written * 100) / contentLength;
        static int lastPercent = -1;
        if (percent >= lastPercent + 10) {
          LOG_INFO(MODULE_OTA, "Progress: %d%% (%d/%d bytes)", percent, written, contentLength);
          lastPercent = percent;
        }
      }
    } else {
      delay(1);
    }
  }
  
  http.end();
  
  if (written == 0) {
    LOG_ERROR(MODULE_OTA, "No data received");
    Update.abort();
    return false;
  }
  
  LOG_INFO(MODULE_OTA, "Downloaded %d bytes", written);
  
  // Finish update
  if (!Update.end()) {
    LOG_ERROR(MODULE_OTA, "Update.end() failed: %s", Update.errorString());
    return false;
  }
  
  if (!Update.isFinished()) {
    LOG_ERROR(MODULE_OTA, "Update not finished");
    Update.abort();
    return false;
  }
  
  LOG_INFO(MODULE_OTA, "Firmware update successful! Rebooting...");
  delay(1000);
  ESP.restart();
  
  return true;
}

bool OTAClient::downloadAndApplyFromURL(const String& url) {
  if (WiFi.status() != WL_CONNECTED) {
    LOG_ERROR(MODULE_OTA, "WiFi not connected, cannot download update");
    return false;
  }
  if (url.length() == 0) {
    LOG_ERROR(MODULE_OTA, "Empty URL");
    return false;
  }

  HTTPClient http;
  http.begin(url);
  http.setTimeout(OTA_TIMEOUT_MS);

  LOG_INFO(MODULE_OTA, "Downloading from %s", url.c_str());

  int httpCode = http.GET();

  if (httpCode != 200) {
    LOG_ERROR(MODULE_OTA, "Failed to download, HTTP code: %d", httpCode);
    http.end();
    return false;
  }

  int contentLength = http.getSize();
  if (contentLength <= 0) {
    LOG_ERROR(MODULE_OTA, "Invalid content length: %d", contentLength);
    http.end();
    return false;
  }
  LOG_INFO(MODULE_OTA, "Content length: %d bytes", contentLength);

  if (contentLength > UPDATE_SIZE_UNKNOWN && contentLength > (ESP.getFreeSketchSpace() - 0x1000)) {
    LOG_ERROR(MODULE_OTA, "Not enough space. Required: %d, Available: %d",
              contentLength, ESP.getFreeSketchSpace());
    http.end();
    return false;
  }

  if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
    LOG_ERROR(MODULE_OTA, "Update.begin() failed: %s", Update.errorString());
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  size_t written = 0;
  uint8_t buffer[1024];

  while (http.connected() && (written < contentLength || contentLength == -1)) {
    size_t available = stream->available();
    if (available) {
      int c = stream->readBytes(buffer, min(available, sizeof(buffer)));
      Update.write(buffer, c);
      written += c;
      if (contentLength > 0) {
        int percent = (written * 100) / contentLength;
        static int lastPercent = -1;
        if (percent >= lastPercent + 10) {
          LOG_INFO(MODULE_OTA, "Progress: %d%% (%d/%d bytes)", percent, written, contentLength);
          lastPercent = percent;
        }
      }
    } else {
      delay(1);
    }
  }

  http.end();

  if (written == 0) {
    LOG_ERROR(MODULE_OTA, "No data received");
    Update.abort();
    return false;
  }
  LOG_INFO(MODULE_OTA, "Downloaded %d bytes", written);

  if (!Update.end()) {
    LOG_ERROR(MODULE_OTA, "Update.end() failed: %s", Update.errorString());
    return false;
  }
  if (!Update.isFinished()) {
    LOG_ERROR(MODULE_OTA, "Update not finished");
    Update.abort();
    return false;
  }
  LOG_INFO(MODULE_OTA, "Update successful! Rebooting...");
  delay(1000);
  ESP.restart();
  return true;
}

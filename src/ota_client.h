#ifndef OTA_CLIENT_H
#define OTA_CLIENT_H

#include <Arduino.h>

struct OTAInfo {
  bool updateAvailable;
  String firmwareName;
  String firmwareTag;
  String firmwareDate;
  String firmwareVersion;
};

class OTAClient {
public:
  /**
   * Check if firmware update is available.
   * Makes HEAD request to https://api.example.com/p1m5/firmware
   * 
   * @param deviceSerial Device serial (e.g., "P1846680")
   * @param currentVersion Current firmware version
   * @param macAddress MAC address string (e.g., "D4:D4:DA:84:66:80")
   * @return OTAInfo with updateAvailable flag
   */
  static OTAInfo checkForUpdate(const String& deviceSerial, const String& currentVersion, const String& macAddress);
  
  /**
   * Download and apply firmware update.
   * Makes GET request and streams to OTA partition.
   * 
   * @param deviceSerial Device serial
   * @param currentVersion Current firmware version
   * @param macAddress MAC address string
   * @return true if update successful, false otherwise
   */
  static bool downloadAndApply(const String& deviceSerial, const String& currentVersion, const String& macAddress);
  
  /**
   * Set custom firmware URL (for testing)
   */
  static void setFirmwareURL(const String& url);

  /**
   * Download and apply firmware from an arbitrary HTTP URL (plain GET).
   * Use for pull-based OTA e.g. from a local server.
   */
  static bool downloadAndApplyFromURL(const String& url);

private:
  static String firmwareURL;
  static bool downloadAndApplyImpl(const String& url, const String* deviceSerial, const String* currentVersion, const String* macAddress);
  static void setUpdateHeaders(class HTTPClient& client, const String& deviceSerial, const String& currentVersion, const String& macAddress);
  static String getMacAddressString();
};

#endif

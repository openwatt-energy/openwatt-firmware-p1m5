#ifndef MQTT_AUTH_H
#define MQTT_AUTH_H

#include <Arduino.h>

class MQTTAuth {
public:
  /**
   * Generate MQTT password from device ID and secret key.
   * Matches Python implementation in mosquitto/mqtt.example.com/setup_auth.py
   * 
   * Algorithm:
   * 1. Concatenate secret_key + device_id
   * 2. SHA256 hash the result
   * 3. Take first 10 bytes
   * 4. Base64 encode
   * 
   * @param deviceId Device ID (e.g., "P1846680")
   * @param secretKey Secret key from configuration
   * @return Base64-encoded password string
   */
  static String generatePassword(const String& deviceId, const String& secretKey);
  
private:
  static String base64Encode(const uint8_t* data, size_t length);
};

#endif

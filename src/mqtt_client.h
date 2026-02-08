#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <Arduino.h>
#include "config.h"

struct MQTTConfig {
  String host;
  uint16_t port;
  String topic;
  String username;  // Device ID
  String secretKey;  // For password generation (not stored in NVS)
  bool useTLS;
};

class MQTTClient {
public:
  static void begin(Preferences& prefs, const String& deviceId, const String& secretKey = "");
  static void loop();
  static void reconnect();
  static bool isConnected();
  static void publish(const String& topic, const String& payload);
  static MQTTConfig getConfig();
  static void setConfig(const MQTTConfig& config);
  static void saveConfig(Preferences& prefs);
  
private:
  static PubSubClient* client;
  static WiFiClientSecure wifiClientSecure;
  static WiFiClient wifiClientPlain;  // For non-TLS fallback
  static MQTTConfig config;
  static String deviceId;
  static String secretKey;
  static bool useSecure;
  static String generatePassword();
};

#endif

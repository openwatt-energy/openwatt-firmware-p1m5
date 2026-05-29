#include "mqtt_client.h"
#include "mqtt_auth.h"
#include "serial_console.h"
#include "debug_logger.h"

#define DEFAULT_MQTT_PORT MQTT_BROKER_PORT

PubSubClient* MQTTClient::client = nullptr;
WiFiClientSecure MQTTClient::wifiClientSecure;
WiFiClient MQTTClient::wifiClientPlain;
MQTTConfig MQTTClient::config;
String MQTTClient::deviceId;
String MQTTClient::secretKey;
bool MQTTClient::useSecure = true;
MQTTMessageCallback MQTTClient::messageCallback = nullptr;

void MQTTClient::begin(Preferences& prefs, const String& devId, const String& secKey) {
  deviceId = devId;
  secretKey = secKey;

  // Force production MQTT settings and save to NVS
  config.host = MQTT_BROKER_HOST;  // mqtt.example.com
  config.port = MQTT_BROKER_PORT;  // 8883
  config.topic = MQTT_DEFAULT_TOPIC;  // P1M5/
  config.username = deviceId;  // Username is the P1 number (e.g., "P19D49B8")
  config.useTLS = true;  // Always use TLS on port 8883

  // Save forced settings to NVS for persistence
  prefs.putString("mqtt_host", config.host);
  prefs.putUShort("mqtt_port", config.port);
  prefs.putString("mqtt_topic", config.topic);
  prefs.putBool("mqtt_use_tls", config.useTLS);
  prefs.putString("mqtt_username", config.username);

  LOG_INFO(MODULE_MQTT, "MQTT settings saved to NVS:");
  LOG_INFO(MODULE_MQTT, "  Host: %s", config.host.c_str());
  LOG_INFO(MODULE_MQTT, "  Port: %d", config.port);
  LOG_INFO(MODULE_MQTT, "  Topic: %s", config.topic.c_str());

  // Determine if we should use TLS based on port
  useSecure = (config.port == 8883) || config.useTLS;

  // Initialize appropriate client
  if (useSecure) {
    // For TLS, skip certificate validation for now (can be added later)
    wifiClientSecure.setInsecure();
    client = new PubSubClient(wifiClientSecure);
  } else {
    client = new PubSubClient(wifiClientPlain);
  }

  if (config.host.length() > 0) {
    client->setServer(config.host.c_str(), config.port);
    // Increase buffer size to handle large JSON payloads (default is only 256 bytes)
    client->setBufferSize(4096);
    LOG_INFO(MODULE_MQTT, "MQTT configured:");
    LOG_INFO(MODULE_MQTT, "  Host: %s:%d", config.host.c_str(), config.port);
    LOG_INFO(MODULE_MQTT, "  Topic: %s", config.topic.c_str());
    LOG_INFO(MODULE_MQTT, "  TLS: %s", useSecure ? "Yes" : "No");
    LOG_INFO(MODULE_MQTT, "  Buffer: 4096 bytes");
  } else {
    LOG_WARN(MODULE_MQTT, "MQTT not configured");
  }
}

void MQTTClient::loop() {
  if (client) {
    client->loop();
  }
}

String MQTTClient::generatePassword() {
  if (secretKey.length() == 0) {
    LOG_ERROR(MODULE_MQTT, "Secret key not set, cannot generate password");
    return String("");
  }
  return MQTTAuth::generatePassword(deviceId, secretKey);
}

void MQTTClient::setMessageCallback(MQTTMessageCallback callback) {
  messageCallback = callback;
}

void MQTTClient::mqttCallback(char* topic, byte* payload, unsigned int length) {
  String payloadStr;
  payloadStr.reserve(length);
  for (unsigned int i = 0; i < length; i++) {
    payloadStr += (char)payload[i];
  }

  String topicStr = String(topic);

  LOG_INFO(MODULE_MQTT, "Message received on topic: %s", topicStr.c_str());

  if (messageCallback) {
    messageCallback(topicStr, payloadStr);
  }
}

void MQTTClient::subscribeToTopics() {
  if (!client || !client->connected()) {
    return;
  }

  // Subscribe to command topic: <topic_prefix><device_id>/cmd
  String cmdTopic = config.topic;
  if (!cmdTopic.endsWith("/")) {
    cmdTopic += "/";
  }
  cmdTopic += deviceId + "/cmd";

  if (client->subscribe(cmdTopic.c_str())) {
    LOG_INFO(MODULE_MQTT, "Subscribed to command topic: %s", cmdTopic.c_str());
  } else {
    LOG_ERROR(MODULE_MQTT, "Failed to subscribe to command topic: %s", cmdTopic.c_str());
  }
}

void MQTTClient::reconnect() {
  if (!client || !config.host.length() > 0) {
    return;
  }

  // Rate limit reconnection attempts (max once every 10 seconds)
  static unsigned long lastReconnectAttempt = 0;
  unsigned long now = millis();
  if (now - lastReconnectAttempt < 10000) {
    return;
  }
  lastReconnectAttempt = now;

  if (!client->connected()) {
    LOG_INFO(MODULE_MQTT, "Attempting MQTT connection to %s:%d...", config.host.c_str(), config.port);

    // Generate password
    String password = generatePassword();
    if (password.length() == 0) {
      LOG_ERROR(MODULE_MQTT, "Failed to generate password");
      return;
    }

    // Connect with username (device ID) and password
    String username = config.username.length() > 0 ? config.username : deviceId;

    if (client->connect(deviceId.c_str(), username.c_str(), password.c_str())) {
      LOG_INFO(MODULE_MQTT, "MQTT connected as %s", username.c_str());
      client->setCallback(mqttCallback);
      subscribeToTopics();
    } else {
      int state = client->state();
      LOG_ERROR(MODULE_MQTT, "MQTT connection failed, state: %d", state);
    }
  }
}

bool MQTTClient::isConnected() {
  return client && client->connected();
}

void MQTTClient::publish(const String& topic, const String& payload) {
  if (client && client->connected() && topic.length() > 0) {
    // topic should be the full path including device ID (e.g., "P1834378/data/readings")
    // config.topic is the base prefix (e.g., "P1M5/" or "P1M5")
    String fullTopic;
    if (config.topic.length() > 0) {
      // Check if config.topic already ends with "/"
      if (config.topic.endsWith("/")) {
        fullTopic = config.topic + topic;
      } else {
        fullTopic = config.topic + "/" + topic;
      }
    } else {
      fullTopic = topic;
    }
    bool result = client->publish(fullTopic.c_str(), payload.c_str());
    if (!result) {
      LOG_WARN(MODULE_MQTT, "Failed to publish to %s", fullTopic.c_str());
    }
  }
}

MQTTConfig MQTTClient::getConfig() {
  return config;
}

void MQTTClient::setConfig(const MQTTConfig& newConfig) {
  config = newConfig;
  useSecure = (config.port == 8883) || config.useTLS;

  // Reinitialize client if needed
  if (client) {
    delete client;
    client = nullptr;
  }

  if (config.host.length() > 0) {
    if (useSecure) {
      wifiClientSecure.setInsecure();
      client = new PubSubClient(wifiClientSecure);
    } else {
      client = new PubSubClient(wifiClientPlain);
    }
    client->setServer(config.host.c_str(), config.port);
    client->setBufferSize(4096);
  }
}

void MQTTClient::saveConfig(Preferences& prefs) {
  prefs.putString("mqtt_host", config.host);
  prefs.putUShort("mqtt_port", config.port);
  prefs.putString("mqtt_topic", config.topic);
  prefs.putString("mqtt_username", config.username);
  prefs.putBool("mqtt_use_tls", config.useTLS);
}

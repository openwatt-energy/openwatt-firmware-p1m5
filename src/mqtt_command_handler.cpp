#include "mqtt_command_handler.h"
#include <ArduinoJson.h>
#include "proxy_http.h"
#include "proxy_modbus.h"
#include "proxy_scanner.h"
#include "debug_logger.h"

#define MODULE_PROXY "PROXY"

void MQTTCommandHandler::handleMessage(const String& topic, const String& payload) {
  // Expected topic structure: P1M5/{deviceId}/cmd
  if (!topic.endsWith("/cmd")) {
    return;
  }

  // Derive response topic by stripping the prefix (e.g., "P1M5/")
  // If topic is "P1M5/P1850D1C/cmd", we want "P1850D1C/cmd_response"
  int lastSlash = topic.lastIndexOf('/');
  if (lastSlash == -1) return;

  int prevSlash = topic.lastIndexOf('/', lastSlash - 1);
  String responseTopic;
  if (prevSlash != -1) {
    responseTopic = topic.substring(prevSlash + 1) + "_response";
  } else {
    responseTopic = topic + "_response";
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    DebugLogger::error(MODULE_PROXY, "Invalid JSON command");
    return;
  }

  if (!doc["action"].is<String>()) {
    DebugLogger::error(MODULE_PROXY, "Command missing action");
    return;
  }

  String action = doc["action"].as<String>();
  JsonObject request = doc.as<JsonObject>();

  if (action == "http_request") {
    ProxyHTTP::handleRequest(request, responseTopic);
  } else if (action == "modbus_read") {
    ProxyModbus::handleRequest(request, responseTopic);
  } else if (action == "scan_ports") {
    ProxyScanner::handleRequest(request, responseTopic);
  } else {
    DebugLogger::warn(MODULE_PROXY, "Unknown action: %s", action.c_str());
  }
}

#include "mqtt_command_handler.h"
#include <ArduinoJson.h>
#include "proxy_http.h"
#include "proxy_modbus.h"
#include "proxy_scanner.h"
#include "debug_logger.h"

#define MODULE_PROXY "PROXY"

void MQTTCommandHandler::handleMessage(const String& topic, const String& payload) {
  // Expected topic structure: openwatt/dongles/{deviceId}/cmd
  if (!topic.endsWith("/cmd")) {
    return;
  }

  // Derive response topic
  String responseTopic = topic + "_response";

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    DebugLogger::error(MODULE_PROXY, "Invalid JSON command");
    return;
  }

  if (!doc.containsKey("action")) {
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

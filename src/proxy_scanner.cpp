#include "proxy_scanner.h"
#include <WiFiClient.h>
#include <vector>
#include "mqtt_client.h"
#include "debug_logger.h"

#define MODULE_PROXY "PROXY"

// Simple state machine for scanning to avoid blocking
static bool scanActive = false;
static String scanReqId;
static String scanResponseTopic;
static String scanBaseIp;
static int scanStartIp;
static int scanEndIp;
static int scanCurrentIp;
static std::vector<int> scanPorts;
static int scanCurrentPortIdx;
static JsonDocument scanResults;
static unsigned long lastScanTick = 0;
static WiFiClient scanClient;

void ProxyScanner::handleRequest(JsonObject request, const String& responseTopic) {
  if (scanActive) {
    DebugLogger::warn(MODULE_PROXY, "Scan already in progress");
    return;
  }

  if (!request.containsKey("req_id") || !request.containsKey("base_ip") ||
      !request.containsKey("start_ip") || !request.containsKey("end_ip") || !request.containsKey("ports")) {
    DebugLogger::error(MODULE_PROXY, "Scan request missing parameters");
    return;
  }

  scanReqId = request["req_id"].as<String>();
  scanResponseTopic = responseTopic;
  scanBaseIp = request["base_ip"].as<String>();
  scanStartIp = request["start_ip"].as<int>();
  scanEndIp = request["end_ip"].as<int>();
  scanCurrentIp = scanStartIp;

  scanPorts.clear();
  JsonArray ports = request["ports"].as<JsonArray>();
  for (JsonVariant v : ports) {
    scanPorts.push_back(v.as<int>());
  }
  scanCurrentPortIdx = 0;

  scanResults.clear();
  scanResults["action"] = "scan_result";
  scanResults["req_id"] = scanReqId;
  scanResults.createNestedArray("found");

  scanActive = true;
  lastScanTick = millis();
  DebugLogger::info(MODULE_PROXY, "Started scan %s on %s%d-%d", scanReqId.c_str(), scanBaseIp.c_str(), scanStartIp, scanEndIp);
}

void ProxyScanner::loop() {
  if (!scanActive) return;

  // Don't monopolize CPU, do one port attempt per loop, every 5ms
  if (millis() - lastScanTick < 5) return;
  lastScanTick = millis();

  IPAddress targetIp;
  targetIp.fromString(scanBaseIp + String(scanCurrentIp));
  int targetPort = scanPorts[scanCurrentPortIdx];

  // Use a very short timeout for local network scanning (50ms)
  if (scanClient.connect(targetIp, targetPort, 50)) {
    DebugLogger::info(MODULE_PROXY, "Found %s:%d", targetIp.toString().c_str(), targetPort);
    JsonObject found = scanResults["found"].as<JsonArray>().add<JsonObject>();
    found["ip"] = targetIp.toString();
    found["port"] = targetPort;
    scanClient.stop();
  }

  // Advance state
  scanCurrentPortIdx++;
  if (scanCurrentPortIdx >= scanPorts.size()) {
    scanCurrentPortIdx = 0;
    scanCurrentIp++;
  }

  if (scanCurrentIp > scanEndIp) {
    // Finished
    String responseStr;
    serializeJson(scanResults, responseStr);
    MQTTClient::publish(scanResponseTopic, responseStr);
    DebugLogger::info(MODULE_PROXY, "Scan %s complete", scanReqId.c_str());
    scanActive = false;
  }
}

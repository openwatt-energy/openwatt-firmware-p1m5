#include "proxy_http.h"
#include <HTTPClient.h>
#include "mqtt_client.h"
#include "debug_logger.h"

#define MODULE_PROXY "PROXY"

void ProxyHTTP::handleRequest(JsonObject request, const String& responseTopic) {
  if (!request.containsKey("url") || !request.containsKey("req_id")) {
    DebugLogger::error(MODULE_PROXY, "HTTP request missing url or req_id");
    return;
  }

  String req_id = request["req_id"].as<String>();
  String url = request["url"].as<String>();
  String method = request.containsKey("method") ? request["method"].as<String>() : "GET";
  method.toUpperCase();

  int timeout_ms = request.containsKey("timeout_ms") ? request["timeout_ms"].as<int>() : 5000;

  DebugLogger::info(MODULE_PROXY, "HTTP %s request to %s (id: %s)", method.c_str(), url.c_str(), req_id.c_str());

  HTTPClient http;
  http.setTimeout(timeout_ms);
  http.begin(url);

  if (request.containsKey("headers") && request["headers"].is<JsonObject>()) {
    JsonObject headers = request["headers"];
    for (JsonPair kv : headers) {
      http.addHeader(kv.key().c_str(), kv.value().as<const char*>());
    }
  }

  int httpCode;
  if (method == "POST" || method == "PUT" || method == "PATCH") {
    String body = request.containsKey("body") ? request["body"].as<String>() : "";
    if (method == "POST") httpCode = http.POST(body);
    else if (method == "PUT") httpCode = http.PUT(body);
    else httpCode = http.PATCH(body);
  } else {
    // Default GET/DELETE etc. HTTPClient doesn't have explicit DELETE method with body, but generic sendRequest
    if (method == "DELETE") httpCode = http.sendRequest("DELETE");
    else httpCode = http.GET();
  }

  String payload = http.getString();
  http.end();

  // Create response
  JsonDocument doc;
  doc["action"] = "http_response";
  doc["req_id"] = req_id;
  doc["status_code"] = httpCode;

  // Only include body if it's reasonable size (MQTT limits)
  if (payload.length() > 0 && payload.length() < 4000) {
    doc["body"] = payload;
  } else if (payload.length() >= 4000) {
    doc["body"] = "{\"error\": \"response too large\"}";
  }

  String responseStr;
  serializeJson(doc, responseStr);

  MQTTClient::publish(responseTopic, responseStr);
  DebugLogger::info(MODULE_PROXY, "HTTP response sent (code: %d)", httpCode);
}

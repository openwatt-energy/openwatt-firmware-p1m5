#include "proxy_modbus.h"
#include <WiFiClient.h>
#include "mqtt_client.h"
#include "debug_logger.h"

#define MODULE_PROXY "PROXY"

// Modbus TCP MBAP Header is 7 bytes
// Transaction ID (2), Protocol ID (2, always 0), Length (2), Unit ID (1)
// Followed by Modbus PDU: Function Code (1), Data (N)

void ProxyModbus::handleRequest(JsonObject request, const String& responseTopic) {
  if (!request.containsKey("req_id") || !request.containsKey("ip") ||
      !request.containsKey("address") || !request.containsKey("function")) {
    DebugLogger::error(MODULE_PROXY, "Modbus request missing parameters");
    return;
  }

  String req_id = request["req_id"].as<String>();
  String ipStr = request["ip"].as<String>();
  int port = request.containsKey("port") ? request["port"].as<int>() : 502;
  uint8_t unit_id = request.containsKey("unit_id") ? request["unit_id"].as<uint8_t>() : 1;
  uint8_t funcCode = request["function"].as<uint8_t>();
  uint16_t address = request["address"].as<uint16_t>();
  uint16_t count = request.containsKey("count") ? request["count"].as<uint16_t>() : 1;

  DebugLogger::info(MODULE_PROXY, "Modbus request %d to %s:%d", funcCode, ipStr.c_str(), port);

  WiFiClient client;
  client.setTimeout(2); // 2 second timeout for modbus device response

  if (!client.connect(ipStr.c_str(), port, 2000)) {
    DebugLogger::error(MODULE_PROXY, "Modbus connection failed to %s", ipStr.c_str());
    return; // Optionally send error response
  }

  uint8_t requestBuf[12];
  uint16_t transId = random(1, 65535);

  // MBAP
  requestBuf[0] = transId >> 8;
  requestBuf[1] = transId & 0xFF;
  requestBuf[2] = 0; // Protocol ID high
  requestBuf[3] = 0; // Protocol ID low
  requestBuf[4] = 0; // Length high (6 bytes follow for read)
  requestBuf[5] = 6; // Length low
  requestBuf[6] = unit_id;

  // PDU
  requestBuf[7] = funcCode;
  requestBuf[8] = address >> 8;
  requestBuf[9] = address & 0xFF;
  requestBuf[10] = count >> 8;
  requestBuf[11] = count & 0xFF;

  client.write(requestBuf, 12);

  // Wait for response header (7 bytes MBAP + 2 bytes PDU min)
  unsigned long startWait = millis();
  while(client.available() < 9 && millis() - startWait < 2000) {
    delay(1);
  }

  JsonDocument doc;
  doc["action"] = "modbus_response";
  doc["req_id"] = req_id;

  if (client.available() >= 9) {
    uint8_t respHeader[9];
    client.read(respHeader, 9);

    // Check if error response (highest bit of func code set)
    if (respHeader[7] == (funcCode | 0x80)) {
      doc["error"] = "exception";
      doc["exception_code"] = respHeader[8];
    } else {
      // It's a valid read response
      uint8_t byteCount = respHeader[8];
      JsonArray data = doc.createNestedArray("data");

      while(client.available() < byteCount && millis() - startWait < 2500) {
        delay(1);
      }

      for (int i=0; i<byteCount; i+=2) {
        if (client.available() >= 2) {
          uint8_t hi = client.read();
          uint8_t lo = client.read();
          uint16_t val = (hi << 8) | lo;
          data.add(val);
        }
      }
    }
  } else {
    doc["error"] = "timeout";
  }

  client.stop();

  String responseStr;
  serializeJson(doc, responseStr);
  MQTTClient::publish(responseTopic, responseStr);
}

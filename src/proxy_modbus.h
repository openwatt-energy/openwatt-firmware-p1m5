#ifndef PROXY_MODBUS_H
#define PROXY_MODBUS_H

#include <Arduino.h>
#include <ArduinoJson.h>

class ProxyModbus {
public:
  static void handleRequest(JsonObject request, const String& responseTopic);
};

#endif

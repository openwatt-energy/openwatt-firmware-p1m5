#ifndef PROXY_SCANNER_H
#define PROXY_SCANNER_H

#include <Arduino.h>
#include <ArduinoJson.h>

class ProxyScanner {
public:
  static void handleRequest(JsonObject request, const String& responseTopic);
  static void loop();
};

#endif

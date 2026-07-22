#ifndef PROXY_HTTP_H
#define PROXY_HTTP_H

#include <Arduino.h>
#include <ArduinoJson.h>

class ProxyHTTP {
public:
  static void handleRequest(JsonObject request, const String& responseTopic);
};

#endif

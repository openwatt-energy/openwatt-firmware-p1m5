#ifndef MQTT_COMMAND_HANDLER_H
#define MQTT_COMMAND_HANDLER_H

#include <Arduino.h>

class MQTTCommandHandler {
public:
  static void handleMessage(const String& topic, const String& payload);
};

#endif

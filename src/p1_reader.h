#ifndef P1_READER_H
#define P1_READER_H

#include <Arduino.h>

// Feature flag - define before including this header if needed
#ifndef ENABLE_P1_READER
#define ENABLE_P1_READER 0
#endif

struct P1Data {
  String equipmentId;
  String timestamp;
  uint8_t tariffIndicator = 0;
  float consumptionT1 = 0.0;
  float productionT1 = 0.0;
  float consumptionT2 = 0.0;
  float productionT2 = 0.0;
  float powerConsumed = 0.0;
  float powerProduced = 0.0;
  float avgDemand = 0.0;
  float maxDemandMonth = 0.0;
  float maxDemand13M = 0.0;
  bool valid = false;
};

class P1Reader {
public:
  static void begin();
  /** Call from main loop() to run Serial2 init after ~5s (avoids WDT in task). */
  static void loop();
  static P1Data parseTelegram(const String& telegram);
  static bool isConnected();
  static unsigned long getBytesReceived();  // Debug: total bytes received on Serial1
  static String getLastRawTelegram();  // Debug: get last raw telegram received

private:
  static void p1ReaderTask(void* pvParameters);
  static bool meterConnected;
  static unsigned long bytesReceived;  // Debug counter
  static String lastRawTelegram;  // Debug: store last raw telegram
};

#endif

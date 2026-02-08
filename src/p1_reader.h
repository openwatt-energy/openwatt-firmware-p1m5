#ifndef P1_READER_H
#define P1_READER_H

#include <Arduino.h>

// Feature flag - define before including this header if needed
#ifndef ENABLE_P1_READER
#define ENABLE_P1_READER 0
#endif

struct P1Data {
  // Identification
  String equipmentId;
  String meterModel;        // 0-0:96.1.4
  String meterVersion;      // 1-0:94.32.1
  String secondId;         // 0-0:96.1.2
  String textMessage;      // 0-0:96.13.0
  
  // Timestamp and tariff
  String timestamp;
  uint8_t tariffIndicator = 0;
  
  // Energy consumption/production (kWh) - 5 tariff periods supported
  float consumptionT1 = 0.0;  // 1-0:1.8.1
  float consumptionT2 = 0.0;  // 1-0:1.8.2
  float consumptionT3 = 0.0;  // 1-0:1.8.3
  float consumptionT4 = 0.0;  // 1-0:1.8.4
  float consumptionT5 = 0.0;  // 1-0:1.8.5
  float productionT1 = 0.0;   // 1-0:2.8.1
  float productionT2 = 0.0;   // 1-0:2.8.2
  float productionT3 = 0.0;   // 1-0:2.8.3
  float productionT4 = 0.0;   // 1-0:2.8.4
  float productionT5 = 0.0;   // 1-0:2.8.5
  
  // Instantaneous power (kW)
  float powerConsumed = 0.0;  // 1-0:1.7.0 - total import
  float powerProduced = 0.0;  // 1-0:2.7.0 - total export
  float powerTotal = 0.0;     // 1-0:61.7.0 - total across all phases
  float avgDemand = 0.0;      // 1-0:1.4.0
  
  // Power import per phase (kW) - ISk5 specific
  float powerImportL1 = 0.0;  // 1-0:21.7.0
  float powerImportL2 = 0.0;  // 1-0:41.7.0
  float powerImportL3 = 0.0;  // 1-0:61.7.0
  
  // Power export per phase (kW) - ISk5/Fluvius specific
  float powerExportL1 = 0.0;  // 1-0:22.7.0
  float powerExportL2 = 0.0;  // 1-0:42.7.0
  float powerExportL3 = 0.0;  // 1-0:62.7.0
  
  // Demand history
  float maxDemandMonth = 0.0;
  float maxDemand13M = 0.0;
  String maxDemandTimestamp;  // 1-0:1.6.0 timestamp
  
  // Voltage (V) - 3 phases
  float voltageL1 = 0.0;  // 1-0:32.7.0
  float voltageL2 = 0.0;  // 1-0:52.7.0
  float voltageL3 = 0.0;  // 1-0:72.7.0
  
  // Current (A) - 3 phases
  float currentL1 = 0.0;  // 1-0:31.7.0
  float currentL2 = 0.0;  // 1-0:51.7.0
  float currentL3 = 0.0;  // 1-0:71.7.0
  
  // Switch position
  int switchPosition = 0;  // 0-0:96.3.10
  
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
  static unsigned long getTelegramCount();  // Debug: number of telegrams received

private:
  static void p1ReaderTask(void* pvParameters);
  static bool meterConnected;
  static unsigned long bytesReceived;  // Debug counter
  static String lastRawTelegram;  // Debug: store last raw telegram
  static unsigned long telegramCount;  // Debug: count telegrams
};

#endif

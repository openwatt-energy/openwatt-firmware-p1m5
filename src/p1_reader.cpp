#include "config.h"  // ENABLE_P1_READER - must be set before p1_reader.h
#include "p1_reader.h"
#include "p1_dispatch.h"
#include "serial_console.h"
#include <HardwareSerial.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Plan-d-io P1-dongle hardware uses Serial1 on GPIO 21/22
#define P1_UART_NUM      1
#define P1_RX_PIN        21
#define P1_TX_PIN        22
#define P1_TRIGGER_PIN   25
#define P1_BAUD_RATE     115200
#define P1_BUFFER_SIZE   2048   // telegram buffer (task stack)

// Feature flag is defined in p1_reader.h

bool P1Reader::meterConnected = false;
unsigned long P1Reader::bytesReceived = 0;
String P1Reader::lastRawTelegram = "";
unsigned long P1Reader::telegramCount = 0;
static bool p1SerialInitDone = false;

static float extractFloatValue(const String& line) {
  int startIdx = line.indexOf('(');
  int endIdx = line.indexOf('*');
  
  if (startIdx == -1 || endIdx == -1) {
    return 0.0;
  }
  
  String valueStr = line.substring(startIdx + 1, endIdx);
  return valueStr.toFloat();
}

static String extractStringValue(const String& line) {
  int startIdx = line.indexOf('(');
  int endIdx = line.indexOf(')');
  
  if (startIdx == -1 || endIdx == -1) {
    return "";
  }
  
  return line.substring(startIdx + 1, endIdx);
}

static int extractIntValue(const String& line) {
  int startIdx = line.indexOf('(');
  int endIdx = line.indexOf(')');
  
  if (startIdx == -1 || endIdx == -1) {
    return 0;
  }
  
  String valueStr = line.substring(startIdx + 1, endIdx);
  return valueStr.toInt();
}

// Calculate CRC16 checksum for P1 telegram validation
static uint16_t calculateCRC16(const char* data, size_t len) {
  uint16_t crc = 0;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (int j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

// Validate CRC16 checksum at end of telegram
static bool validateCRC(const String& telegram) {
  int crcIdx = telegram.indexOf('!');
  if (crcIdx == -1) {
    return false;  // No CRC marker found
  }
  
  // Extract CRC from telegram (4 hex characters after '!')
  if (telegram.length() < crcIdx + 5) {
    return false;  // CRC not complete
  }
  
  String crcStr = telegram.substring(crcIdx + 1, crcIdx + 5);
  uint16_t receivedCRC = 0;
  
  // Parse hex CRC
  for (int i = 0; i < 4; i++) {
    char c = crcStr.charAt(i);
    uint8_t nibble = 0;
    if (c >= '0' && c <= '9') {
      nibble = c - '0';
    } else if (c >= 'A' && c <= 'F') {
      nibble = c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
      nibble = c - 'a' + 10;
    } else {
      return false;  // Invalid hex character
    }
    receivedCRC = (receivedCRC << 4) | nibble;
  }
  
  // Calculate CRC for telegram up to '!' (excluding CRC itself)
  String telegramWithoutCRC = telegram.substring(0, crcIdx + 1);
  uint16_t calculatedCRC = calculateCRC16(telegramWithoutCRC.c_str(), telegramWithoutCRC.length());
  
  return (calculatedCRC == receivedCRC);
}

P1Data P1Reader::parseTelegram(const String& telegram) {
  P1Data data;
  data.valid = false;
  
  // Validate telegram starts with '/'
  if (telegram.length() == 0 || telegram[0] != '/') {
    SerialConsole::println("Invalid telegram: doesn't start with '/'");
    return data;
  }
  
  // Validate CRC checksum (log warning but don't fail - some meters have different CRC implementations)
  if (!validateCRC(telegram)) {
    SerialConsole::println("Warning: CRC checksum mismatch - accepting telegram anyway");
    // Continue parsing even if CRC fails - some Belgian meters use different CRC methods
  }
  
  // Handle both \r\n and \n line endings
  String normalizedTelegram = telegram;
  normalizedTelegram.replace("\r\n", "\n");
  normalizedTelegram.replace("\r", "\n");
  
  int startIdx = 0;
  int endIdx = 0;
  int lineCount = 0;
  
  // Parse line by line
  while ((endIdx = normalizedTelegram.indexOf('\n', startIdx)) != -1 || 
         (startIdx < normalizedTelegram.length() && endIdx == -1)) {
    
    // Handle last line (might not end with \n)
    if (endIdx == -1) {
      endIdx = normalizedTelegram.length();
    }
    
    String line = normalizedTelegram.substring(startIdx, endIdx);
    line.trim();
    
    // Skip empty lines and CRC line
    if (line.length() == 0 || line[0] == '!') {
      startIdx = endIdx + 1;
      continue;
    }
    
    lineCount++;
    
    // Parse known OBIS codes (using indexOf for flexibility with unknown codes)
    // Verbose logging disabled - uncomment for debugging
    // SerialConsole::println("Parsing line [" + String(lineCount) + "]: " + line);
    
    if (line.indexOf("0-0:96.1.1(") != -1) {
      data.equipmentId = extractStringValue(line);
      SerialConsole::println("  -> Equipment ID: " + data.equipmentId);
    }
    else if (line.indexOf("0-0:1.0.0(") != -1) {
      data.timestamp = extractStringValue(line);
      SerialConsole::println("  -> Timestamp: " + data.timestamp);
    }
    else if (line.indexOf("0-0:96.14.0(") != -1) {
      data.tariffIndicator = extractStringValue(line).toInt();
      SerialConsole::println("  -> Tariff: " + String(data.tariffIndicator));
    }
    else if (line.indexOf("1-0:1.8.1(") != -1) {
      data.consumptionT1 = extractFloatValue(line);
      SerialConsole::println("  -> Consumption T1: " + String(data.consumptionT1));
    }
    else if (line.indexOf("1-0:2.8.1(") != -1) {
      data.productionT1 = extractFloatValue(line);
      SerialConsole::println("  -> Production T1: " + String(data.productionT1));
    }
    else if (line.indexOf("1-0:1.8.2(") != -1) {
      data.consumptionT2 = extractFloatValue(line);
      SerialConsole::println("  -> Consumption T2: " + String(data.consumptionT2));
    }
    else if (line.indexOf("1-0:1.8.3(") != -1) {
      data.consumptionT3 = extractFloatValue(line);
      SerialConsole::println("  -> Consumption T3: " + String(data.consumptionT3));
    }
    else if (line.indexOf("1-0:1.8.4(") != -1) {
      data.consumptionT4 = extractFloatValue(line);
      SerialConsole::println("  -> Consumption T4: " + String(data.consumptionT4));
    }
    else if (line.indexOf("1-0:1.8.5(") != -1) {
      data.consumptionT5 = extractFloatValue(line);
      SerialConsole::println("  -> Consumption T5: " + String(data.consumptionT5));
    }
    else if (line.indexOf("1-0:2.8.2(") != -1) {
      data.productionT2 = extractFloatValue(line);
      SerialConsole::println("  -> Production T2: " + String(data.productionT2));
    }
    else if (line.indexOf("1-0:2.8.3(") != -1) {
      data.productionT3 = extractFloatValue(line);
      SerialConsole::println("  -> Production T3: " + String(data.productionT3));
    }
    else if (line.indexOf("1-0:2.8.4(") != -1) {
      data.productionT4 = extractFloatValue(line);
      SerialConsole::println("  -> Production T4: " + String(data.productionT4));
    }
    else if (line.indexOf("1-0:2.8.5(") != -1) {
      data.productionT5 = extractFloatValue(line);
      SerialConsole::println("  -> Production T5: " + String(data.productionT5));
    }
    else if (line.indexOf("1-0:1.7.0(") != -1) {
      data.powerConsumed = extractFloatValue(line);
      SerialConsole::println("  -> Power Consumed: " + String(data.powerConsumed));
    }
    else if (line.indexOf("1-0:2.7.0(") != -1) {
      data.powerProduced = extractFloatValue(line);
      SerialConsole::println("  -> Power Produced: " + String(data.powerProduced));
    }
    else if (line.indexOf("1-0:1.4.0(") != -1) {
      data.avgDemand = extractFloatValue(line);
      SerialConsole::println("  -> Avg Demand: " + String(data.avgDemand));
    }
    else if (line.indexOf("1-0:1.6.0(") != -1) {
      // Format: 1-0:1.6.0(00.010*kW)(251016170000)
      // Extract first value (before second parenthesis)
      int firstParen = line.indexOf('(');
      int firstStar = line.indexOf('*', firstParen);
      if (firstParen != -1 && firstStar != -1) {
        String valueStr = line.substring(firstParen + 1, firstStar);
        data.maxDemandMonth = valueStr.toFloat();
      }
    }
    else if (line.indexOf("0-0:98.1.0(") != -1) {
      data.maxDemand13M = extractFloatValue(line);
    }
    // Additional OBIS codes for complete Fluvius meter support
    else if (line.indexOf("0-0:96.1.4(") != -1) {
      data.meterModel = extractStringValue(line);
      SerialConsole::println("  -> Meter Model: " + data.meterModel);
    }
    else if (line.indexOf("1-0:94.32.1(") != -1) {
      data.meterVersion = extractStringValue(line);
      SerialConsole::println("  -> Meter Version: " + data.meterVersion);
    }
    else if (line.indexOf("0-0:96.1.2(") != -1) {
      data.secondId = extractStringValue(line);
      SerialConsole::println("  -> Second ID: " + data.secondId);
    }
    else if (line.indexOf("0-0:96.13.0(") != -1) {
      data.textMessage = extractStringValue(line);
      SerialConsole::println("  -> Text Message: " + data.textMessage);
    }
    // Voltage readings (3 phases)
    else if (line.indexOf("1-0:32.7.0(") != -1) {
      data.voltageL1 = extractFloatValue(line);
      SerialConsole::println("  -> Voltage L1: " + String(data.voltageL1) + " V");
    }
    else if (line.indexOf("1-0:52.7.0(") != -1) {
      data.voltageL2 = extractFloatValue(line);
      SerialConsole::println("  -> Voltage L2: " + String(data.voltageL2) + " V");
    }
    else if (line.indexOf("1-0:72.7.0(") != -1) {
      data.voltageL3 = extractFloatValue(line);
      SerialConsole::println("  -> Voltage L3: " + String(data.voltageL3) + " V");
    }
    // Current readings (3 phases)
    else if (line.indexOf("1-0:31.7.0(") != -1) {
      data.currentL1 = extractFloatValue(line);
      SerialConsole::println("  -> Current L1: " + String(data.currentL1) + " A");
    }
    else if (line.indexOf("1-0:51.7.0(") != -1) {
      data.currentL2 = extractFloatValue(line);
      SerialConsole::println("  -> Current L2: " + String(data.currentL2) + " A");
    }
    else if (line.indexOf("1-0:71.7.0(") != -1) {
      data.currentL3 = extractFloatValue(line);
      SerialConsole::println("  -> Current L3: " + String(data.currentL3) + " A");
    }
    // Power readings (3 phases) - ISk5/Fluvius specific
    else if (line.indexOf("1-0:61.7.0(") != -1) {
      data.powerTotal = extractFloatValue(line);
      data.powerImportL3 = data.powerTotal;  // 61.7.0 is phase 3 import
      SerialConsole::println("  -> Total Power: " + String(data.powerTotal) + " kW");
    }
    // Power import per phase (kW)
    else if (line.indexOf("1-0:21.7.0(") != -1) {
      data.powerImportL1 = extractFloatValue(line);
      SerialConsole::println("  -> Power Import L1: " + String(data.powerImportL1) + " kW");
    }
    else if (line.indexOf("1-0:41.7.0(") != -1) {
      data.powerImportL2 = extractFloatValue(line);
      SerialConsole::println("  -> Power Import L2: " + String(data.powerImportL2) + " kW");
    }
    // Power export per phase (kW) - for solar production
    else if (line.indexOf("1-0:22.7.0(") != -1) {
      data.powerExportL1 = extractFloatValue(line);
      SerialConsole::println("  -> Power Export L1: " + String(data.powerExportL1) + " kW");
    }
    else if (line.indexOf("1-0:42.7.0(") != -1) {
      data.powerExportL2 = extractFloatValue(line);
      SerialConsole::println("  -> Power Export L2: " + String(data.powerExportL2) + " kW");
    }
    else if (line.indexOf("1-0:62.7.0(") != -1) {
      data.powerExportL3 = extractFloatValue(line);
      SerialConsole::println("  -> Power Export L3: " + String(data.powerExportL3) + " kW");
    }
    // Switch position
    else if (line.indexOf("0-0:96.3.10(") != -1) {
      data.switchPosition = extractIntValue(line);
      SerialConsole::println("  -> Switch Position: " + String(data.switchPosition));
    }
    // Max demand with timestamp (format: 1-0:1.6.0(timestamp)(value))
    else if (line.indexOf("1-0:1.6.0(") != -1) {
      // Extract both timestamp and value
      int firstParen = line.indexOf('(');
      int secondParen = line.indexOf('(', firstParen + 1);
      int thirdParen = line.indexOf('(', secondParen + 1);
      int starIdx = line.indexOf('*', thirdParen);
      
      if (firstParen != -1 && secondParen != -1) {
        data.maxDemandTimestamp = line.substring(firstParen + 1, secondParen);
        SerialConsole::println("  -> Max Demand Timestamp: " + data.maxDemandTimestamp);
      }
      if (thirdParen != -1 && starIdx != -1) {
        String valueStr = line.substring(thirdParen + 1, starIdx);
        data.maxDemandMonth = valueStr.toFloat();
        SerialConsole::println("  -> Max Demand Value: " + String(data.maxDemandMonth) + " kW");
      }
    }
    // Unknown OBIS codes - log for debugging (max 20 unique codes per telegram to avoid spam)
    else if (line.length() > 0 && lineCount <= 20) {
      // Check if it looks like an OBIS code (contains ':' and '(')
      if (line.indexOf(':') != -1 && line.indexOf('(') != -1) {
        SerialConsole::println("  -> UNPARSED: " + line);
      }
    }
    
    startIdx = endIdx + 1;
    
    // Safety check: prevent infinite loop
    if (lineCount > 100) {
      SerialConsole::println("Warning: Telegram has more than 100 lines, stopping parse");
      break;
    }
  }
  
  // Validate we got at least some data - be lenient for Fluvius meters
  bool hasAnyData = (data.equipmentId.length() > 0 || data.timestamp.length() > 0 ||
                     data.consumptionT1 > 0 || data.consumptionT2 > 0 || 
                     data.productionT1 > 0 || data.productionT2 > 0);
  
  if (!hasAnyData) {
    SerialConsole::println("Warning: Telegram parsed but no data found");
  } else {
    SerialConsole::println("Parsed - Equipment: " + data.equipmentId + " T1: " + String(data.consumptionT1) + " T2: " + String(data.consumptionT2));
  }
  
  data.valid = hasAnyData;
  return data;
}

void P1Reader::p1ReaderTask(void* pvParameters) {
  SerialConsole::println("[P1 Task] Starting (waiting for Serial1 init from main loop)...");
  while (!p1SerialInitDone) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  SerialConsole::println("P1 reader task started");
  
  static char telegram[P1_BUFFER_SIZE];
  int telegramLen = 0;
  bool receiving = false;
  unsigned long lastCharTime = 0;
  const unsigned long TELEGRAM_TIMEOUT = 5000;  // 5 second timeout
  
  while (true) {
    // Feed watchdog frequently but check serial more aggressively
    yield();
    
    // Read all available bytes in a tight loop (don't delay between bytes)
    while (Serial1.available() > 0) {
      char c = Serial1.read();
      bytesReceived++;
      lastCharTime = millis();
      
       if (c == '/') {
        // Start of new telegram
        if (!receiving) {
          // Log timestamp when new telegram starts
          unsigned long now = millis();
          static unsigned long lastTelegramTime = 0;
          if (lastTelegramTime > 0) {
            unsigned long interval = now - lastTelegramTime;
            SerialConsole::println("[P1] New telegram started. Interval since last: " + String(interval) + "ms");
          }
          lastTelegramTime = now;
        }
        receiving = true;
        telegramLen = 0;
        telegram[0] = '/';
        telegram[1] = '\0';
        telegramLen = 1;
        meterConnected = true;
      }
      else if (receiving && telegramLen < P1_BUFFER_SIZE - 2) {
        telegram[telegramLen++] = c;
        telegram[telegramLen] = '\0';
        
        if (c == '!') {
          // Read CRC (4 hex characters) - with timeout protection
          unsigned long crcStartTime = millis();
          const unsigned long CRC_TIMEOUT = 1000;  // 1 second timeout for CRC
          
          for (int i = 0; i < 4 && telegramLen < P1_BUFFER_SIZE - 2; i++) {
            // Wait for data with timeout
            while (!Serial1.available() && (millis() - crcStartTime < CRC_TIMEOUT)) {
              yield();
              vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            
            if (Serial1.available()) {
              telegram[telegramLen++] = Serial1.read();
              telegram[telegramLen] = '\0';
            } else {
              // Timeout reading CRC
              SerialConsole::println("ERROR: Timeout reading CRC");
              receiving = false;
              telegramLen = 0;
              telegram[0] = '\0';
              break;
            }
          }
          
          if (receiving && telegramLen > 5) {
            // Complete telegram received, validate and parse
            telegramCount++;
            String telegramStr = String(telegram);
            lastRawTelegram = telegramStr;  // Store for API access
            
            P1Data data = parseTelegram(telegramStr);
            
            if (data.valid) {
              // Only log every 10th telegram to reduce spam (update ~10 seconds)
              if (telegramCount % 10 == 0) {
                SerialConsole::println("[P1] Telegram #" + String(telegramCount) + " received (" + String(telegramLen) + " bytes), data valid");
              }
              onP1DataReceived(data);
            } else {
              SerialConsole::println("[P1] Telegram #" + String(telegramCount) + " invalid");
            }
          }
          
          receiving = false;
          telegramLen = 0;
          telegram[0] = '\0';
        }
      } else if (receiving && telegramLen >= P1_BUFFER_SIZE - 2) {
        // Buffer overflow protection
        SerialConsole::println("ERROR: Telegram buffer overflow! Max size: " + String(P1_BUFFER_SIZE));
        receiving = false;
        telegramLen = 0;
        telegram[0] = '\0';
        meterConnected = false;  // Mark as disconnected due to error
      }
    }
    
    // Small delay when no data available to prevent task spinning
    if (Serial1.available() == 0) {
      vTaskDelay(5 / portTICK_PERIOD_MS);  // Only 5ms delay when idle
    }
    
    // Timeout handling - if we're receiving but no data for 5 seconds, reset
    if (receiving && lastCharTime > 0) {
      unsigned long now = millis();
      // Handle millis() overflow
      unsigned long elapsed = (now >= lastCharTime) ? (now - lastCharTime) : (ULONG_MAX - lastCharTime + now);
      
      if (elapsed > TELEGRAM_TIMEOUT) {
        SerialConsole::println("ERROR: Telegram timeout (" + String(elapsed) + "ms) - resetting");
        receiving = false;
        telegramLen = 0;
        telegram[0] = '\0';
        lastCharTime = 0;
        meterConnected = false;  // Mark as disconnected due to timeout
      }
    }
    
    // Check if meter disconnected (no data for extended period)
    if (!receiving && meterConnected && lastCharTime > 0) {
      unsigned long now = millis();
      unsigned long elapsed = (now >= lastCharTime) ? (now - lastCharTime) : (ULONG_MAX - lastCharTime + now);
      
      if (elapsed > (TELEGRAM_TIMEOUT * 2)) {
        // No data for 2x timeout period, consider meter disconnected
        meterConnected = false;
        SerialConsole::println("Meter appears disconnected (no data for " + String(elapsed) + "ms)");
        lastCharTime = 0;
      }
    }
  }
}

void P1Reader::begin() {
  #if ENABLE_P1_READER
    // Setup trigger pin for continuous meter reading (HIGH = continuous mode)
    pinMode(P1_TRIGGER_PIN, OUTPUT);
    digitalWrite(P1_TRIGGER_PIN, HIGH);
    SerialConsole::println("P1 trigger pin (GPIO 25) set to HIGH for continuous mode");
    delay(100);  // Give meter time to wake up
    
    // Deferred init: task does Serial1.setRxBufferSize/begin() after 4s to avoid WDT in setup()
    SerialConsole::println("Creating P1 reader task (Serial1 init deferred)...");
    BaseType_t taskResult = xTaskCreate(
      p1ReaderTask,
      "P1_Reader",
      8192,
      NULL,
      3,
      NULL
    );
    
    if (taskResult == pdPASS) {
      SerialConsole::println("P1 reader task created");
    } else {
      SerialConsole::println("Failed to create P1 reader task!");
    }
  #else
    SerialConsole::println("P1 UART disabled (ENABLE_P1_READER=0)");
  #endif
}

void P1Reader::loop() {
  #if ENABLE_P1_READER
    if (p1SerialInitDone) return;
    static unsigned long lastCheck = 0;
    if (lastCheck == 0) lastCheck = millis();
    if (millis() - lastCheck < 5000) return;
    lastCheck = millis();
    SerialConsole::println("[P1] Serial1 init from main loop (GPIO 21/22, 115200)...");
    // Initialize Serial1 with larger RX buffer to prevent overflow at 115200 baud
    Serial1.setRxBufferSize(1024);  // Increase from default 256 to 1024 bytes
    Serial1.begin(P1_BAUD_RATE, SERIAL_8N1, P1_RX_PIN, P1_TX_PIN);
    delay(100);
    p1SerialInitDone = true;
    SerialConsole::println("[P1] Serial1 ready.");
  #endif
}

bool P1Reader::isConnected() {
  return meterConnected;
}

unsigned long P1Reader::getBytesReceived() {
  return bytesReceived;
}

String P1Reader::getLastRawTelegram() {
  return lastRawTelegram;
}

unsigned long P1Reader::getTelegramCount() {
  return telegramCount;
}

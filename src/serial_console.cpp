#include "serial_console.h"
#include "config.h"
#include <esp_system.h>

void SerialConsole::begin() {
  Serial.begin(115200);
  delay(500);

  while (Serial.available()) { Serial.read(); }
  Serial.setTimeout(1000);

  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);

  Serial.println();
  Serial.println("=================================");
  Serial.println(CUSTOMER_DISPLAY_NAME " P1 Reader  " FIRMWARE_VERSION);
  Serial.printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println("Serial: 115200 8N1 (use this baud in terminal)");
  Serial.println("=================================");
  Serial.flush();
}

void SerialConsole::println(const String& msg) {
  Serial.println(msg);
  Serial.flush();
}

void SerialConsole::print(const String& msg) {
  Serial.print(msg);
  Serial.flush();
}

void SerialConsole::flush() {
  Serial.flush();
}

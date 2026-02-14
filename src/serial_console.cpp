#include "serial_console.h"
#include "config.h"

void SerialConsole::begin() {
  Serial.begin(115200);
  delay(500);

  while (Serial.available()) { Serial.read(); }
  Serial.setTimeout(1000);

  Serial.println();
  Serial.println("=================================");
  Serial.println(CUSTOMER_DISPLAY_NAME " P1 Reader  " FIRMWARE_VERSION);
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

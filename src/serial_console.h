#ifndef SERIAL_CONSOLE_H
#define SERIAL_CONSOLE_H

#include <Arduino.h>

class SerialConsole {
public:
  static void begin();
  static void println(const String& msg);
  static void print(const String& msg);
  static void flush();
};

#endif

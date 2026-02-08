#ifndef LED_HANDLER_H
#define LED_HANDLER_H

#include <Arduino.h>

enum class LEDState {
  OFF,
  ON,
  BLINKING_BLUE,
  BLINKING_YELLOW,
  SOLID_YELLOW
};

class LEDHandler {
public:
  static void begin();
  static void setState(LEDState state);
  static void loop();
  static void setAPMode(bool apMode);
  static void setOTAUpdate(bool updating);
  
private:
  static LEDState currentState;
  static bool apModeActive;
  static bool otaUpdating;
  static unsigned long lastBlinkTime;
  static bool blinkState;
  
  static void updateLED();
};

#endif

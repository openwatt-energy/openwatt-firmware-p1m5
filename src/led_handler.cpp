#include "led_handler.h"

// Built-in LED pin (GPIO 2 on most ESP32 boards)
// For M5Stack Core ESP32 with RGB LED, you may need to use FastLED or NeoPixel library
#define LED_BUILTIN_PIN 2

LEDState LEDHandler::currentState = LEDState::OFF;
bool LEDHandler::apModeActive = false;
bool LEDHandler::otaUpdating = false;
unsigned long LEDHandler::lastBlinkTime = 0;
bool LEDHandler::blinkState = false;

void LEDHandler::begin() {
  // Initialize LED pin
  pinMode(LED_BUILTIN_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN_PIN, LOW);
  
  // Start with LED on (device powered)
  setState(LEDState::ON);
}

void LEDHandler::setState(LEDState state) {
  currentState = state;
  lastBlinkTime = millis();
  blinkState = false;
  updateLED();
}

void LEDHandler::setAPMode(bool apMode) {
  apModeActive = apMode;
  if (apMode && !otaUpdating) {
    setState(LEDState::SOLID_YELLOW);
  } else if (!apMode && !otaUpdating) {
    setState(LEDState::ON);
  }
}

void LEDHandler::setOTAUpdate(bool updating) {
  otaUpdating = updating;
  if (updating) {
    setState(LEDState::BLINKING_BLUE);
  } else {
    // Return to previous state
    if (apModeActive) {
      setState(LEDState::SOLID_YELLOW);
    } else {
      setState(LEDState::ON);
    }
  }
}

void LEDHandler::loop() {
  unsigned long now = millis();
  
  // Handle blinking states
  if (currentState == LEDState::BLINKING_BLUE || currentState == LEDState::BLINKING_YELLOW) {
    if (now - lastBlinkTime >= 500) {  // 500ms blink interval
      blinkState = !blinkState;
      lastBlinkTime = now;
      updateLED();
    }
  }
}

void LEDHandler::updateLED() {
  // For single LED (GPIO 2), we use different blink patterns:
  // - ON: solid on
  // - BLINKING_BLUE: fast blink (OTA update)
  // - BLINKING_YELLOW: medium blink (not used currently)
  // - SOLID_YELLOW: solid on (AP mode - same as ON for single LED)
  
  switch (currentState) {
    case LEDState::OFF:
      digitalWrite(LED_BUILTIN_PIN, LOW);
      break;
      
    case LEDState::ON:
      digitalWrite(LED_BUILTIN_PIN, HIGH);
      break;
      
    case LEDState::BLINKING_BLUE:
      // Fast blink for OTA (blue = fast)
      digitalWrite(LED_BUILTIN_PIN, blinkState ? HIGH : LOW);
      break;
      
    case LEDState::BLINKING_YELLOW:
      // Medium blink (yellow = medium speed)
      digitalWrite(LED_BUILTIN_PIN, blinkState ? HIGH : LOW);
      break;
      
    case LEDState::SOLID_YELLOW:
      // Solid on for AP mode (yellow = solid)
      digitalWrite(LED_BUILTIN_PIN, HIGH);
      break;
  }
}

#include "led_handler.h"

// M5Stack Atom has RGB LED on GPIO 27
#define LED_PIN 27
#define NUM_LEDS 1

// Static members
LEDStatus LEDHandler::currentStatus = LEDStatus::BOOTING;
CRGB LEDHandler::leds[NUM_LEDS];
unsigned long LEDHandler::lastUpdate = 0;
int LEDHandler::blinkPhase = 0;
bool LEDHandler::blinkState = false;

void LEDHandler::begin() {
  // Initialize FastLED
  FastLED.addLeds<SK6812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(128);  // 50% brightness

  // Start with booting state (red)
  setBooting();
}

void LEDHandler::loop() {
  unsigned long now = millis();

  // Update LED based on current status
  if (now - lastUpdate >= 100) {  // Update every 100ms
    updateLED();
    lastUpdate = now;
  }

  FastLED.show();
}

void LEDHandler::setBooting() {
  currentStatus = LEDStatus::BOOTING;
}

void LEDHandler::setError() {
  currentStatus = LEDStatus::ERROR_UNKNOWN;
}

void LEDHandler::setOTAUpdate(bool inProgress) {
  if (inProgress) {
    currentStatus = LEDStatus::OTA_IN_PROGRESS;
  } else {
    // Return to previous state logic
    setWiFiStatus(true, true, true);  // Default to all good
  }
}

void LEDHandler::setAPMode(bool active, bool meterConnected, bool wifiAvailable) {
  if (!active) return;

  if (!meterConnected) {
    currentStatus = LEDStatus::AP_MODE_NO_METER;
  } else if (!wifiAvailable) {
    currentStatus = LEDStatus::AP_MODE_NO_WIFI;
  } else {
    currentStatus = LEDStatus::AP_MODE_READY;
  }
}

void LEDHandler::setWiFiStatus(bool connected, bool meterConnected, bool cloudConnected) {
  if (!connected) {
    // Should be in AP mode, handled separately
    return;
  }

  if (!meterConnected && !cloudConnected) {
    currentStatus = LEDStatus::WIFI_NO_METER_CLOUD;
  } else if (!meterConnected) {
    currentStatus = LEDStatus::WIFI_NO_METER;
  } else if (!cloudConnected) {
    currentStatus = LEDStatus::WIFI_NO_CLOUD;
  } else {
    currentStatus = LEDStatus::WIFI_METER_CLOUD;
  }
}

void LEDHandler::setColor(CRGB color) {
  leds[0] = color;
  FastLED.show();
}

void LEDHandler::setStatus(LEDStatus status) {
  currentStatus = status;
}

LEDStatus LEDHandler::getStatus() {
  return currentStatus;
}

void LEDHandler::updateLED() {
  static unsigned long patternStart = 0;
  unsigned long now = millis();

  switch (currentStatus) {
    case LEDStatus::BOOTING:
      // Red continuous
      applyPattern(COLOR_RED, BlinkPattern::SOLID);
      break;

    case LEDStatus::ERROR_UNKNOWN:
      // Red fast blink
      applyPattern(COLOR_RED, BlinkPattern::FAST);
      break;

    case LEDStatus::OTA_IN_PROGRESS:
      // Orange continuous
      applyPattern(COLOR_ORANGE, BlinkPattern::SOLID);
      break;

    case LEDStatus::AP_MODE_NO_METER:
    case LEDStatus::AP_MODE_NO_WIFI:
      // Blue slow blink
      applyPattern(COLOR_BLUE, BlinkPattern::SLOW);
      break;

    case LEDStatus::AP_MODE_READY:
      // Blue continuous
      applyPattern(COLOR_BLUE, BlinkPattern::SOLID);
      break;

    case LEDStatus::WIFI_NO_METER:
      // Green slow blink
      applyPattern(COLOR_GREEN, BlinkPattern::SLOW);
      break;

    case LEDStatus::WIFI_NO_CLOUD:
      // Green double blink
      applyPattern(COLOR_GREEN, BlinkPattern::DOUBLE);
      break;

    case LEDStatus::WIFI_METER_CLOUD:
      // Green continuous
      applyPattern(COLOR_GREEN, BlinkPattern::SOLID);
      break;

    case LEDStatus::WIFI_NO_METER_CLOUD:
      // Green triple blink
      applyPattern(COLOR_GREEN, BlinkPattern::TRIPLE);
      break;
  }
}

void LEDHandler::applyPattern(CRGB color, BlinkPattern pattern) {
  static unsigned long patternStart = 0;
  static int phase = 0;
  unsigned long now = millis();

  switch (pattern) {
    case BlinkPattern::SOLID:
      leds[0] = color;
      break;

    case BlinkPattern::SLOW:
      // ~1Hz (500ms on, 500ms off)
      if ((now / 500) % 2 == 0) {
        leds[0] = color;
      } else {
        leds[0] = COLOR_OFF;
      }
      break;

    case BlinkPattern::FAST:
      // ~5Hz (100ms on, 100ms off)
      if ((now / 100) % 2 == 0) {
        leds[0] = color;
      } else {
        leds[0] = COLOR_OFF;
      }
      break;

    case BlinkPattern::DOUBLE:
      // Two quick blinks, then pause
      {
        unsigned long cycle = now % 1500;  // 1.5 second cycle
        if (cycle < 200 || (cycle > 400 && cycle < 600)) {
          leds[0] = color;
        } else {
          leds[0] = COLOR_OFF;
        }
      }
      break;

    case BlinkPattern::TRIPLE:
      // Three quick blinks, then pause
      {
        unsigned long cycle = now % 2000;  // 2 second cycle
        if (cycle < 200 || (cycle > 400 && cycle < 600) || (cycle > 800 && cycle < 1000)) {
          leds[0] = color;
        } else {
          leds[0] = COLOR_OFF;
        }
      }
      break;
  }
}

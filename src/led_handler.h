#ifndef LED_HANDLER_H
#define LED_HANDLER_H

#include <Arduino.h>
#include <FastLED.h>

// LED color definitions (RGB)
#define COLOR_RED     CRGB(255, 0, 0)
#define COLOR_ORANGE  CRGB(255, 165, 0)
#define COLOR_YELLOW  CRGB(255, 255, 0)
#define COLOR_GREEN   CRGB(0, 255, 0)
#define COLOR_BLUE    CRGB(0, 0, 255)
#define COLOR_OFF     CRGB(0, 0, 0)

// LED states per OpenWatt documentation
enum class LEDStatus {
  BOOTING,           // Red continuous - device starting
  ERROR_UNKNOWN,     // Red fast blink - unknown error
  OTA_IN_PROGRESS,   // Orange continuous - firmware update
  AP_MODE_NO_METER,  // Blue slow blink - AP mode, no P1 data
  AP_MODE_NO_WIFI,   // Blue slow blink - AP mode, WiFi issue
  AP_MODE_READY,     // Blue continuous - AP mode ready for config
  WIFI_NO_METER,     // Green slow blink - WiFi ok, no P1 data
  WIFI_NO_CLOUD,     // Green double blink - WiFi ok, no MQTT
  WIFI_METER_CLOUD,  // Green continuous - All good
  WIFI_NO_METER_CLOUD // Green triple blink - No P1, no MQTT
};

// Blink patterns
enum class BlinkPattern {
  SOLID,       // Continuous
  SLOW,        // ~1Hz
  FAST,        // ~5Hz
  DOUBLE,      // Two blinks
  TRIPLE       // Three blinks
};

class LEDHandler {
public:
  static void begin();
  static void loop();

  // Status setters
  static void setBooting();
  static void setError();
  static void setOTAUpdate(bool inProgress);
  static void setAPMode(bool active, bool meterConnected, bool wifiAvailable);
  static void setWiFiStatus(bool connected, bool meterConnected, bool cloudConnected);

  // Manual control
  static void setColor(CRGB color);
  static void setStatus(LEDStatus status);
  static LEDStatus getStatus();

private:
  static LEDStatus currentStatus;
  static CRGB leds[1];  // Single LED for M5Stack Atom
  static unsigned long lastUpdate;
  static int blinkPhase;
  static bool blinkState;

  static void updateLED();
  static void applyPattern(CRGB color, BlinkPattern pattern);
};

#endif

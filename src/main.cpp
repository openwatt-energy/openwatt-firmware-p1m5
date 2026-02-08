/**
 * OpenWatt P1 Reader Firmware
 * Based on reverse engineering of Xenn P1 Dongle
 * 
 * Features:
 * - Reads DSMR P1 telegrams from Belgian smart meters
 * - WiFi connectivity (STA + AP modes)
 * - MQTT publishing
 * - REST API for configuration
 * - WebSocket for real-time data
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Preferences.h>
#include <MD5Builder.h>
#include <nvs.h>

// Include config first to define feature flags
#include "config.h"

#include "serial_console.h"
#include "wifi_manager.h"
#include "p1_reader.h"
#include "p1_dispatch.h"
#include "web_api.h"
#include "led_handler.h"
#if ENABLE_MQTT
#include "mqtt_client.h"
#endif
#if ENABLE_OTA
#include "ota_update.h"
#endif

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

Preferences preferences;
AsyncWebServer server(80);
AsyncWebSocket ws("/api/live");

// ============================================================================
// STATE VARIABLES
// ============================================================================

struct DeviceState {
  String deviceId;
  String serialNumber;
} state;

P1Data latestData;

// ============================================================================
// DEVICE ID & PASSWORD GENERATION
// ============================================================================

String getDeviceId() {
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  
  char deviceId[16];
  sprintf(deviceId, "P1%02X%02X%02X", mac[3], mac[4], mac[5]);
  
  return String(deviceId);
}

String getDeviceName() {
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  
  char deviceName[32];
  sprintf(deviceName, "OpenWatt-P1%02X%02X%02X", mac[3], mac[4], mac[5]);
  
  return String(deviceName);
}

// ============================================================================
// FORWARD DECLARATIONS (for onP1DataReceived)
// ============================================================================

void publishToMQTT(const P1Data& data);
void broadcastToWebSocket(const P1Data& data);

// ============================================================================
// MQTT PUBLISHING
// ============================================================================

#if ENABLE_MQTT
void publishToMQTT(const P1Data& data) {
  if (!MQTTClient::isConnected()) {
    return;
  }
  
  JsonDocument doc;
  
  doc["equipment_id"] = data.equipmentId;
  doc["timestamp"] = data.timestamp;
  doc["tariff"] = data.tariffIndicator;
  
  JsonObject consumption = doc["consumption"].to<JsonObject>();
  consumption["tariff1"] = data.consumptionT1;
  consumption["tariff2"] = data.consumptionT2;
  
  JsonObject production = doc["production"].to<JsonObject>();
  production["tariff1"] = data.productionT1;
  production["tariff2"] = data.productionT2;
  
  JsonObject power = doc["power"].to<JsonObject>();
  power["consumed"] = data.powerConsumed;
  power["produced"] = data.powerProduced;
  
  JsonObject demand = doc["demand"].to<JsonObject>();
  demand["current"] = data.avgDemand;
  demand["max_month"] = data.maxDemandMonth;
  demand["max_13months"] = data.maxDemand13M;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  MQTTConfig config = MQTTClient::getConfig();
  MQTTClient::publish(config.topic, jsonString);
}
#else
void publishToMQTT(const P1Data& data) {
  (void)data;
}
#endif

// Called from P1 reader task when a valid telegram is received.
void onP1DataReceived(const P1Data& data) {
  latestData = data;
  WebAPI::setLatestData(data);
  publishToMQTT(data);
  broadcastToWebSocket(data);
}

// ============================================================================
// WEBSOCKET BROADCASTING
// ============================================================================

void broadcastToWebSocket(const P1Data& data) {
  JsonDocument doc;
  
  doc["0-0:96.1.1"] = data.equipmentId;
  doc["0-0:1.0.0"] = data.timestamp;
  doc["0-0:96.14.0"] = data.tariffIndicator;
  doc["1-0:1.8.1"] = data.consumptionT1;
  doc["1-0:2.8.1"] = data.productionT1;
  doc["1-0:1.8.2"] = data.consumptionT2;
  doc["1-0:2.8.2"] = data.productionT2;
  doc["1-0:1.7.0"] = data.powerConsumed;
  doc["1-0:2.7.0"] = data.powerProduced;
  doc["1-0:1.4.0"] = data.avgDemand;
  doc["1-0:1.6.0"] = data.maxDemandMonth;
  doc["0-0:98.1.0"] = data.maxDemand13M;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  ws.textAll(jsonString);
}

// ============================================================================
// WEBSOCKET HANDLER
// ============================================================================

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    SerialConsole::println("WebSocket client connected: " + String(client->id()));
  }
  else if (type == WS_EVT_DISCONNECT) {
    SerialConsole::println("WebSocket client disconnected: " + String(client->id()));
  }
}

// ============================================================================
// SETUP & MAIN LOOP
// ============================================================================

void setup() {
  // Initialize LED first (device powered)
  LEDHandler::begin();
  
  // Initialize Serial Console
  SerialConsole::begin();
  
  SerialConsole::print("Firmware: ");
  SerialConsole::println(FIRMWARE_VERSION);
  SerialConsole::println("");
  
  // Create any missing NVS keys via C API so Preferences never sees NOT_FOUND (it logs even with defaults)
  {
    nvs_handle_t h;
    esp_err_t err = nvs_open("openwatt", NVS_READWRITE, &h);
    if (err == ESP_OK) {
      const char* keys[] = {"email", "wifi_ssid", "wifi_password", "mqtt_host", "mqtt_topic", "mqtt_secret_key", "_init_done"};
      for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        size_t len = 0;
        if (nvs_get_str(h, keys[i], NULL, &len) == ESP_ERR_NVS_NOT_FOUND) {
          nvs_set_str(h, keys[i], (strcmp(keys[i], "_init_done") == 0) ? "1" : "");
        }
      }
      nvs_commit(h);
      nvs_close(h);
      SerialConsole::println("NVS: keys ensured (no NOT_FOUND spam)");
    } else {
      SerialConsole::println("NVS: nvs_open failed, err=" + String((int)err) + " - NOT_FOUND logs may appear");
    }
  }

  SerialConsole::println("Initializing NVS...");
  bool nvsOk = preferences.begin("openwatt", false);
  if (!nvsOk) {
    SerialConsole::println("ERROR: NVS init failed!");
    delay(1000);
  }
  
  // Generate device ID and serial number
  state.deviceId = getDeviceId();
  state.serialNumber = getDeviceName();
  
  SerialConsole::println("Device Info:");
  SerialConsole::println("  ID: " + state.deviceId);
  SerialConsole::println("  Serial: " + state.serialNumber);
  SerialConsole::println("");
  yield();
  delay(100);
  
  // Setup WiFi
  SerialConsole::println("Setting up WiFi...");
  SerialConsole::flush();
  yield();
  WiFiManager::begin(preferences, state.deviceId);
  // Set LED to yellow (AP mode active)
  LEDHandler::setAPMode(true);
  yield();
  delay(100);

  // ArduinoOTA: listen on port 3232 for pio run -t upload --upload-port <device-ip>
  ArduinoOTA.setHostname(state.serialNumber.c_str());
  ArduinoOTA.begin();
  SerialConsole::println("ArduinoOTA ready (port 3232)");
  yield();
  delay(100);
  
  // Setup MQTT (if enabled)
  #if ENABLE_MQTT
  SerialConsole::println("Setting up MQTT...");
  SerialConsole::flush();
  yield();
  // Get secret key from NVS (or use test key for development)
  String mqttSecretKey = preferences.getString("mqtt_secret_key", "");
  if (mqttSecretKey.length() == 0) {
    mqttSecretKey = "test_secret_key_for_development";
    SerialConsole::println("MQTT: Using test secret key");
  }
  MQTTClient::begin(preferences, state.deviceId, mqttSecretKey);
  yield();
  delay(100);
  #else
  SerialConsole::println("MQTT disabled (ENABLE_MQTT=0)");
  yield();
  delay(100);
  #endif
  
  // Setup WebSocket
  SerialConsole::println("Setting up WebSocket...");
  SerialConsole::flush();
  yield();
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  yield();
  delay(100);
  
  // Setup REST API
  SerialConsole::println("Setting up REST API...");
  SerialConsole::flush();
  yield();
  WebAPI::setup(server, preferences, state.deviceId, state.serialNumber);
  yield();
  delay(100);
  
  // Start HTTP server
  SerialConsole::println("Starting HTTP server...");
  SerialConsole::flush();
  yield();
  server.begin();
  SerialConsole::println("HTTP server started on port 80");
  SerialConsole::flush();
  yield();
  delay(100);
  
  // Setup P1 Reader
  SerialConsole::println("Setting up P1 Reader...");
  SerialConsole::flush();
  yield();
  P1Reader::begin();
  yield();
  delay(100);
  
  // Setup OTA Updates (if enabled)
  #if ENABLE_OTA
  SerialConsole::println("Setting up OTA...");
  SerialConsole::flush();
  yield();
  OTAUpdate::begin(state.deviceId, FIRMWARE_VERSION);
  yield();
  delay(100);
  #else
  SerialConsole::println("OTA disabled (ENABLE_OTA=0)");
  yield();
  delay(100);
  #endif
  
  SerialConsole::println("");
  SerialConsole::println("*** Starting OpenWatt application ***");
  SerialConsole::println("Web UI: http://" + WiFiManager::getAPIP());
  SerialConsole::println("");
  SerialConsole::flush();
  yield();
}

void loop() {
  yield();
  ArduinoOTA.handle();
  yield();
  P1Reader::loop();  // Serial1 init after ~5s (avoids WDT in P1 task)
  yield();
  LEDHandler::loop();
  yield();
  
  // Maintain MQTT connection (if enabled)
  #if ENABLE_MQTT
  MQTTClient::reconnect();
  MQTTClient::loop();
  yield();
  #endif
  
  // Handle OTA updates (if enabled)
  #if ENABLE_OTA
  OTAUpdate::loop();
  yield();
  #endif
  
  // Cleanup WebSocket
  ws.cleanupClients();
  yield();
  
  // Check WiFi connection status periodically
  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck > 10000) {
    lastWiFiCheck = millis();
    if (WiFiManager::isConnected() && WiFiManager::getIP().length() > 0) {
      SerialConsole::println("WiFi connected: " + WiFiManager::getIP());
      SerialConsole::flush();
      // WiFi connected - turn off AP mode LED (back to ON)
      LEDHandler::setAPMode(false);
    } else {
      // WiFi not connected - ensure AP mode LED is on
      LEDHandler::setAPMode(true);
    }
    yield();
  }
  
  delay(10);
}

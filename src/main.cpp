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

// Forward declarations
String getFingerprint();

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
  sprintf(deviceName, "%s%02X%02X%02X", AP_SSID_PREFIX, mac[3], mac[4], mac[5]);
  
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
// Helper to generate UUID v4 format
String generateUUID() {
  char uuid[37];
  const char* hex = "0123456789ABCDEF";
  uint8_t bytes[16];
  
  // Generate random bytes
  for (int i = 0; i < 16; i++) {
    bytes[i] = random(256);
  }
  
  // Set version (4) and variant (10xxxxxx)
  bytes[6] = (bytes[6] & 0x0F) | 0x40;
  bytes[8] = (bytes[8] & 0x3F) | 0x80;
  
  // Format as UUID string
  sprintf(uuid, "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
    bytes[0], bytes[1], bytes[2], bytes[3],
    bytes[4], bytes[5], bytes[6], bytes[7],
    bytes[8], bytes[9], bytes[10], bytes[11],
    bytes[12], bytes[13], bytes[14], bytes[15]);
  
  return String(uuid);
}

void publishToMQTT(const P1Data& data) {
  if (!MQTTClient::isConnected()) {
    return;
  }
  
  // Check publish interval
  static unsigned long lastMQTTPublish = 0;
  unsigned long now = millis();
  if (now - lastMQTTPublish < MQTT_PUBLISH_INTERVAL_MS) {
    return;  // Too soon, skip this publish
  }
  lastMQTTPublish = now;
  
  JsonDocument doc;
  
  // Unix timestamp
  doc["timestamp"] = millis() / 1000;
  
  // Timestamp from meter
  doc["0-0:1.0.0"] = data.timestamp;
  
  // Equipment identifier
  doc["0-0:96.1.1"] = data.equipmentId;
  doc["0-0:96.1.2"] = data.secondId;
  doc["0-0:96.1.4"] = data.meterModel;
  doc["0-0:96.13.0"] = data.textMessage;
  doc["0-0:96.14.0"] = String(data.tariffIndicator);
  doc["0-0:96.3.10"] = String(data.switchPosition);
  
  // Device control switches (default to 0 if not available)
  doc["0-1:96.3.10"] = "0";
  doc["0-2:96.3.10"] = "0";
  doc["0-3:96.3.10"] = "0";
  doc["0-4:96.3.10"] = "0";
  
  // Limiter
  JsonObject t17_0_0 = doc["0-0:17.0.0"].to<JsonObject>();
  t17_0_0["value"] = 99.999;
  t17_0_0["unit"] = "kW";
  
  // Average demand
  JsonObject t1_4_0 = doc["1-0:1.4.0"].to<JsonObject>();
  t1_4_0["value"] = data.avgDemand;
  t1_4_0["unit"] = "kW";
  
  // Max demand with capture time
  JsonObject t1_6_0 = doc["1-0:1.6.0"].to<JsonObject>();
  t1_6_0["value"] = data.maxDemandMonth;
  t1_6_0["unit"] = "kW";
  t1_6_0["capture_time"] = data.maxDemandTimestamp;
  if (data.maxDemandTimestamp.length() >= 12) {
    // Convert YYMMDDhhmmss to timestamp (simplified)
    int year = data.maxDemandTimestamp.substring(0, 2).toInt();
    year += (year >= 50) ? 1900 : 2000;
    t1_6_0["timestamp"] = year * 10000000000ULL + data.maxDemandTimestamp.substring(2).toInt();
  }
  
  // Active power
  JsonObject t1_7_0 = doc["1-0:1.7.0"].to<JsonObject>();
  t1_7_0["value"] = data.powerConsumed;
  t1_7_0["unit"] = "kW";
  
  // Energy consumption T1
  JsonObject t1_8_1 = doc["1-0:1.8.1"].to<JsonObject>();
  t1_8_1["value"] = data.consumptionT1;
  t1_8_1["unit"] = "kWh";
  
  // Energy consumption T2
  JsonObject t1_8_2 = doc["1-0:1.8.2"].to<JsonObject>();
  t1_8_2["value"] = data.consumptionT2;
  t1_8_2["unit"] = "kWh";
  
  // Energy consumption T3 (optional - may not be present on all meters)
  if (data.consumptionT3 > 0) {
    JsonObject t1_8_3 = doc["1-0:1.8.3"].to<JsonObject>();
    t1_8_3["value"] = data.consumptionT3;
    t1_8_3["unit"] = "kWh";
  }
  
  // Energy consumption T4 (optional - may not be present on all meters)
  if (data.consumptionT4 > 0) {
    JsonObject t1_8_4 = doc["1-0:1.8.4"].to<JsonObject>();
    t1_8_4["value"] = data.consumptionT4;
    t1_8_4["unit"] = "kWh";
  }
  
  // Energy consumption T5 (optional - may not be present on all meters)
  if (data.consumptionT5 > 0) {
    JsonObject t1_8_5 = doc["1-0:1.8.5"].to<JsonObject>();
    t1_8_5["value"] = data.consumptionT5;
    t1_8_5["unit"] = "kWh";
  }
  
  // Active power export
  JsonObject t2_7_0 = doc["1-0:2.7.0"].to<JsonObject>();
  t2_7_0["value"] = data.powerProduced;
  t2_7_0["unit"] = "kW";
  
  // Energy production T1
  JsonObject t2_8_1 = doc["1-0:2.8.1"].to<JsonObject>();
  t2_8_1["value"] = data.productionT1;
  t2_8_1["unit"] = "kWh";
  
  // Energy production T2
  JsonObject t2_8_2 = doc["1-0:2.8.2"].to<JsonObject>();
  t2_8_2["value"] = data.productionT2;
  t2_8_2["unit"] = "kWh";
  
  // Energy production T3 (optional - may not be present on all meters)
  if (data.productionT3 > 0) {
    JsonObject t2_8_3 = doc["1-0:2.8.3"].to<JsonObject>();
    t2_8_3["value"] = data.productionT3;
    t2_8_3["unit"] = "kWh";
  }
  
  // Energy production T4 (optional - may not be present on all meters)
  if (data.productionT4 > 0) {
    JsonObject t2_8_4 = doc["1-0:2.8.4"].to<JsonObject>();
    t2_8_4["value"] = data.productionT4;
    t2_8_4["unit"] = "kWh";
  }
  
  // Energy production T5 (optional - may not be present on all meters)
  if (data.productionT5 > 0) {
    JsonObject t2_8_5 = doc["1-0:2.8.5"].to<JsonObject>();
    t2_8_5["value"] = data.productionT5;
    t2_8_5["unit"] = "kWh";
  }
  
  // Power import L1
  JsonObject t21_7_0 = doc["1-0:21.7.0"].to<JsonObject>();
  t21_7_0["value"] = data.powerImportL1;
  t21_7_0["unit"] = "kW";
  
  // Power export L1
  JsonObject t22_7_0 = doc["1-0:22.7.0"].to<JsonObject>();
  t22_7_0["value"] = data.powerExportL1;
  t22_7_0["unit"] = "kW";
  
  // Fuse limit
  JsonObject t31_4_0 = doc["1-0:31.4.0"].to<JsonObject>();
  t31_4_0["value"] = 999.99;
  t31_4_0["unit"] = "A";
  
  // Current L1
  JsonObject t31_7_0 = doc["1-0:31.7.0"].to<JsonObject>();
  t31_7_0["value"] = data.currentL1;
  t31_7_0["unit"] = "A";
  
  // Voltage L1
  JsonObject t32_7_0 = doc["1-0:32.7.0"].to<JsonObject>();
  t32_7_0["value"] = data.voltageL1;
  t32_7_0["unit"] = "V";
  
  // Power import L2
  JsonObject t41_7_0 = doc["1-0:41.7.0"].to<JsonObject>();
  t41_7_0["value"] = data.powerImportL2;
  t41_7_0["unit"] = "kW";
  
  // Power export L2
  JsonObject t42_7_0 = doc["1-0:42.7.0"].to<JsonObject>();
  t42_7_0["value"] = data.powerExportL2;
  t42_7_0["unit"] = "kW";
  
  // Current L2
  JsonObject t51_7_0 = doc["1-0:51.7.0"].to<JsonObject>();
  t51_7_0["value"] = data.currentL2;
  t51_7_0["unit"] = "A";
  
  // Voltage L2
  JsonObject t52_7_0 = doc["1-0:52.7.0"].to<JsonObject>();
  t52_7_0["value"] = data.voltageL2;
  t52_7_0["unit"] = "V";
  
  // Power import L3
  JsonObject t61_7_0 = doc["1-0:61.7.0"].to<JsonObject>();
  t61_7_0["value"] = data.powerImportL3;
  t61_7_0["unit"] = "kW";
  
  // Power export L3
  JsonObject t62_7_0 = doc["1-0:62.7.0"].to<JsonObject>();
  t62_7_0["value"] = data.powerExportL3;
  t62_7_0["unit"] = "kW";
  
  // Current L3
  JsonObject t71_7_0 = doc["1-0:71.7.0"].to<JsonObject>();
  t71_7_0["value"] = data.currentL3;
  t71_7_0["unit"] = "A";
  
  // Voltage L3
  JsonObject t72_7_0 = doc["1-0:72.7.0"].to<JsonObject>();
  t72_7_0["value"] = data.voltageL3;
  t72_7_0["unit"] = "V";
  
  // Meter version
  doc["1-0:94.32.1"] = data.meterVersion;
  
  // Metadata
  doc["id"] = generateUUID();
  doc["dongle_id"] = state.deviceId;
  doc["sent_timestamp"] = millis() / 1000;
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Publish to device-specific topic: <device_id>/data/readings
  // (MQTTClient::publish() will add the P1M5/ prefix)
  String topic = state.deviceId + "/data/readings";
  MQTTClient::publish(topic, jsonString);
}

// Publish device status (config log format - Xenn compatible)
void publishMQTTStatus() {
  if (!MQTTClient::isConnected()) {
    return;
  }
  
  // Get or initialize reboot count from NVS
  static int rebootCount = 0;
  static bool rebootCountLoaded = false;
  if (!rebootCountLoaded) {
    rebootCountLoaded = true;
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) == ESP_OK) {
      rebootCount = nvs_get_i32(h, NVS_KEY_REBOOT_COUNT, 0);
      rebootCount++;
      nvs_set_i32(h, NVS_KEY_REBOOT_COUNT, rebootCount);
      nvs_commit(h);
      nvs_close(h);
    }
  }
  
  JsonDocument doc;
  
  // Generate UUID for message ID
  doc["id"] = generateUUID();
  
  // Device identification (Xenn format)
  doc["dongle_id"] = state.deviceId;
  doc["friendly_name"] = "Config log";
  doc["sensorId"] = "config";
  doc["timestamp"] = millis() / 1000;
  doc["hostname"] = state.deviceId;
  
  // System stats
  doc["reboots"] = rebootCount;
  doc["uptime"] = millis() / 1000;  // seconds
  doc["dongle_ip"] = WiFiManager::getIP();
  doc["fw_ver"] = FIRMWARE_VERSION;
  
  // Customer info
  doc["customer"] = CUSTOMER_NAME;
  doc["fingerprint"] = getFingerprint();
  
  // Throttle setting (configurable)
  doc["rtl_throttle"] = 5;  // Default throttle value
  
  // User config (from NVS if available)
  doc["email"] = "";  // Could be loaded from NVS
  
  // MQTT config
  MQTTConfig mqttCfg = MQTTClient::getConfig();
  doc["mqtt_host"] = mqttCfg.host;
  doc["mqtt_port"] = String(mqttCfg.port);
  doc["mqtt_user"] = state.deviceId;
  doc["mqtt_pfix"] = mqttCfg.topic + state.deviceId;
  
  // WiFi config
  doc["wifi_ssid"] = WiFiManager::getConnectedSSID();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  // Publish to config topic (Xenn compatible)
  String topic = state.deviceId + "/sys/config";
  MQTTClient::publish(topic, jsonString);
  
  SerialConsole::println("MQTT: Published config: fw=" + String(FIRMWARE_VERSION));
}
#else
void publishToMQTT(const P1Data& data) {
  (void)data;
}
#endif

// Called from P1 reader task when a valid telegram received.
void onP1DataReceived(const P1Data& data) {
  // Merge data - only update fields that have values in the new telegram
  // This handles meters that send different telegram types (energy vs voltage/current)
  if (data.equipmentId.length() > 0) latestData.equipmentId = data.equipmentId;
  if (data.timestamp.length() > 0) latestData.timestamp = data.timestamp;
  if (data.tariffIndicator > 0) latestData.tariffIndicator = data.tariffIndicator;
  if (data.consumptionT1 > 0) latestData.consumptionT1 = data.consumptionT1;
  if (data.consumptionT2 > 0) latestData.consumptionT2 = data.consumptionT2;
  if (data.productionT1 > 0) latestData.productionT1 = data.productionT1;
  if (data.productionT2 > 0) latestData.productionT2 = data.productionT2;
  if (data.powerConsumed > 0) latestData.powerConsumed = data.powerConsumed;
  if (data.powerProduced > 0) latestData.powerProduced = data.powerProduced;
  if (data.powerImportL1 >= 0) latestData.powerImportL1 = data.powerImportL1;
  if (data.powerImportL2 >= 0) latestData.powerImportL2 = data.powerImportL2;
  if (data.powerImportL3 >= 0) latestData.powerImportL3 = data.powerImportL3;
  if (data.powerExportL1 >= 0) latestData.powerExportL1 = data.powerExportL1;
  if (data.powerExportL2 >= 0) latestData.powerExportL2 = data.powerExportL2;
  if (data.powerExportL3 >= 0) latestData.powerExportL3 = data.powerExportL3;
  if (data.avgDemand > 0) latestData.avgDemand = data.avgDemand;
  if (data.maxDemandMonth > 0) latestData.maxDemandMonth = data.maxDemandMonth;
  if (data.maxDemand13M > 0) latestData.maxDemand13M = data.maxDemand13M;
  if (data.maxDemandTimestamp.length() > 0) latestData.maxDemandTimestamp = data.maxDemandTimestamp;
  
  // Identification
  if (data.meterModel.length() > 0) latestData.meterModel = data.meterModel;
  if (data.meterVersion.length() > 0) latestData.meterVersion = data.meterVersion;
  if (data.secondId.length() > 0) latestData.secondId = data.secondId;
  if (data.textMessage.length() > 0) latestData.textMessage = data.textMessage;
  
  // Voltage (accept 0 values as they are valid readings)
  if (data.voltageL1 >= 0) latestData.voltageL1 = data.voltageL1;
  if (data.voltageL2 >= 0) latestData.voltageL2 = data.voltageL2;
  if (data.voltageL3 >= 0) latestData.voltageL3 = data.voltageL3;
  
  // Current (accept 0 values as they are valid readings)
  if (data.currentL1 >= 0) latestData.currentL1 = data.currentL1;
  if (data.currentL2 >= 0) latestData.currentL2 = data.currentL2;
  if (data.currentL3 >= 0) latestData.currentL3 = data.currentL3;
  if (data.currentTotal >= 0) latestData.currentTotal = data.currentTotal;
  
  // Switch position
  if (data.switchPosition >= 0) latestData.switchPosition = data.switchPosition;
  
  latestData.valid = true;
  
  WebAPI::setLatestData(latestData);
  publishToMQTT(latestData);
  broadcastToWebSocket(latestData);
}

// ============================================================================
// WEBSOCKET BROADCASTING
// ============================================================================

void broadcastToWebSocket(const P1Data& data) {
  JsonDocument doc;
  
  // Add server timestamp so UI can see update frequency even if meter data unchanged
  doc["last_update"] = String(millis());
  
  // Identification
  doc["0-0:96.1.1"] = data.equipmentId;
  doc["0-0:96.1.4"] = data.meterModel;
  doc["1-0:94.32.1"] = data.meterVersion;
  doc["0-0:96.1.2"] = data.secondId;
  doc["0-0:96.13.0"] = data.textMessage;
  
  // Timestamp and tariff
  doc["0-0:1.0.0"] = data.timestamp;
  doc["0-0:96.14.0"] = data.tariffIndicator;
  
  // Energy consumption/production (kWh)
  doc["1-0:1.8.1"] = data.consumptionT1;
  doc["1-0:2.8.1"] = data.productionT1;
  doc["1-0:1.8.2"] = data.consumptionT2;
  doc["1-0:2.8.2"] = data.productionT2;
  
  // Instantaneous power (kW) - total and per-phase
  doc["1-0:1.7.0"] = data.powerConsumed;
  doc["1-0:2.7.0"] = data.powerProduced;
  doc["1-0:1.4.0"] = data.avgDemand;
  doc["1-0:21.7.0"] = data.powerImportL1;  // L1 import
  doc["1-0:41.7.0"] = data.powerImportL2;  // L2 import
  doc["1-0:61.7.0"] = data.powerImportL3;  // L3 import
  
  // Demand history
  doc["1-0:1.6.0"] = data.maxDemandMonth;
  doc["1-0:1.6.0_timestamp"] = data.maxDemandTimestamp;
  doc["0-0:98.1.0"] = data.maxDemand13M;
  
  // Voltage (V) - 3 phases
  doc["1-0:32.7.0"] = data.voltageL1;
  doc["1-0:52.7.0"] = data.voltageL2;
  doc["1-0:72.7.0"] = data.voltageL3;
  
  // Current (A) - 3 phases
  doc["1-0:31.7.0"] = data.currentL1;
  doc["1-0:51.7.0"] = data.currentL2;
  doc["1-0:71.7.0"] = data.currentL3;
  
  // Switch position
  doc["0-0:96.3.10"] = data.switchPosition;
  
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
    // Send initial data immediately
    if (latestData.valid) {
      broadcastToWebSocket(latestData);
    }
  }
  else if (type == WS_EVT_DISCONNECT) {
    SerialConsole::println("WebSocket client disconnected: " + String(client->id()));
  }
  else if (type == WS_EVT_PONG) {
    // Client responded to ping, connection is alive
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
      const char* keys[] = {"email", "wifi_ssid", "wifi_password", "mqtt_host", "mqtt_topic", "_init_done"};
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
  // Set LED to blue (AP mode active, trying to connect)
  LEDHandler::setAPMode(true, false, true);
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
  // Use hardcoded production secret key from config.h
  String mqttSecretKey = SALT_STRING;
  SerialConsole::println("MQTT: Using hardcoded secret key");
  // Set default MQTT topic if not configured
  if (preferences.getString("mqtt_topic", "").length() == 0) {
    preferences.putString("mqtt_topic", "P1M5/");
    SerialConsole::println("MQTT: Set default topic to P1M5/");
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
  
  // Publish status periodically (firmware version, uptime, etc.)
  static unsigned long lastStatusPublish = 0;
  if (millis() - lastStatusPublish > MQTT_STATUS_INTERVAL_MS) {
    lastStatusPublish = millis();
    publishMQTTStatus();
  }
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
      // WiFi connected - update LED status
      bool meterConnected = P1Reader::isConnected();
      bool cloudConnected = MQTTClient::isConnected();
      LEDHandler::setWiFiStatus(true, meterConnected, cloudConnected);
    } else {
      // WiFi not connected - ensure AP mode LED is on
      bool meterConnected = P1Reader::isConnected();
      LEDHandler::setAPMode(true, meterConnected, false);
    }
    yield();
  }
  
  delay(10);
}

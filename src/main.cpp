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
// WiFiManager removed - using manual WiFi setup for custom UI
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Preferences.h>
#include <MD5Builder.h>
#include "web_ui.h"

// ============================================================================
// CONFIGURATION
// ============================================================================

#define FIRMWARE_VERSION "v1.0.0-openwatt"
#define DEVICE_NAME_PREFIX "OpenWatt-P1"
#define AP_SSID_PREFIX "OpenWatt-P1"
#define SALT_STRING "CHANGE_ME_SALT"

// UART Configuration (P1 Port)
#define P1_UART_NUM      2
#define P1_RX_PIN        16        // Adjust for your M5Stack
#define P1_TX_PIN        17        // Not used
#define P1_BAUD_RATE     115200    // Belgian meters
#define P1_BUFFER_SIZE   2048

// Feature flags
#define ENABLE_P1_READER 0  // Disabled until Serial2 issue is resolved

// Default configuration
#define DEFAULT_AP_IP    "192.168.4.1"
#define DEFAULT_MQTT_PORT 1883

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

Preferences preferences;
AsyncWebServer server(80);
AsyncWebSocket ws("/api/live");
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// ============================================================================
// STATE VARIABLES
// ============================================================================

struct DeviceState {
  bool wifiConnected = false;
  bool meterConnected = false;
  bool cloudConnected = false;
  String deviceId;
  String serialNumber;
} state;

struct MQTTConfig {
  String host;
  uint16_t port = DEFAULT_MQTT_PORT;
  String topic;
} mqttConfig;

struct P1Data {
  String equipmentId;
  String timestamp;
  uint8_t tariffIndicator = 0;
  float consumptionT1 = 0.0;
  float productionT1 = 0.0;
  float consumptionT2 = 0.0;
  float productionT2 = 0.0;
  float powerConsumed = 0.0;
  float powerProduced = 0.0;
  float avgDemand = 0.0;
  float maxDemandMonth = 0.0;
  float maxDemand13M = 0.0;
  bool valid = false;
} latestData;

// ============================================================================
// DEVICE ID & PASSWORD GENERATION
// ============================================================================

String getDeviceId() {
  uint8_t mac[6];
  
  // Read MAC address
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

String generatePassword(const String& deviceId) {
  // Based on reverse engineering: deviceId + SALT → MD5 hash
  String combined = deviceId + String(SALT_STRING);
  
  MD5Builder md5;
  md5.begin();
  md5.add(combined);
  md5.calculate();
  
  return md5.toString();
}

// ============================================================================
// P1 TELEGRAM PARSER
// ============================================================================

float extractFloatValue(const String& line) {
  // Format: "1-0:1.8.1(001234.567*kWh)"
  // Extract: 001234.567
  
  int startIdx = line.indexOf('(');
  int endIdx = line.indexOf('*');
  
  if (startIdx == -1 || endIdx == -1) {
    return 0.0;
  }
  
  String valueStr = line.substring(startIdx + 1, endIdx);
  return valueStr.toFloat();
}

String extractStringValue(const String& line) {
  // Format: "0-0:96.1.1(XMX5LGBBFG1009325509)"
  
  int startIdx = line.indexOf('(');
  int endIdx = line.indexOf(')');
  
  if (startIdx == -1 || endIdx == -1) {
    return "";
  }
  
  return line.substring(startIdx + 1, endIdx);
}

P1Data parseTelegram(const String& telegram) {
  P1Data data;
  data.valid = false;
  
  // Validate structure
  if (telegram.length() == 0 || telegram[0] != '/') {
    Serial.println("⚠️  Invalid telegram: doesn't start with '/'");
    return data;
  }
  
  // Split into lines and parse
  int startIdx = 0;
  int endIdx = 0;
  
  while ((endIdx = telegram.indexOf('\n', startIdx)) != -1) {
    String line = telegram.substring(startIdx, endIdx);
    line.trim();
    
    // Parse each OBIS code
    if (line.indexOf("0-0:96.1.1(") != -1) {
      data.equipmentId = extractStringValue(line);
    }
    else if (line.indexOf("0-0:1.0.0(") != -1) {
      data.timestamp = extractStringValue(line);
    }
    else if (line.indexOf("0-0:96.14.0(") != -1) {
      data.tariffIndicator = extractStringValue(line).toInt();
    }
    else if (line.indexOf("1-0:1.8.1(") != -1) {
      data.consumptionT1 = extractFloatValue(line);
    }
    else if (line.indexOf("1-0:2.8.1(") != -1) {
      data.productionT1 = extractFloatValue(line);
    }
    else if (line.indexOf("1-0:1.8.2(") != -1) {
      data.consumptionT2 = extractFloatValue(line);
    }
    else if (line.indexOf("1-0:2.8.2(") != -1) {
      data.productionT2 = extractFloatValue(line);
    }
    else if (line.indexOf("1-0:1.7.0(") != -1) {
      data.powerConsumed = extractFloatValue(line);
    }
    else if (line.indexOf("1-0:2.7.0(") != -1) {
      data.powerProduced = extractFloatValue(line);
    }
    else if (line.indexOf("1-0:1.4.0(") != -1) {
      data.avgDemand = extractFloatValue(line);
    }
    else if (line.indexOf("1-0:1.6.0(") != -1) {
      data.maxDemandMonth = extractFloatValue(line);
    }
    else if (line.indexOf("0-0:98.1.0(") != -1) {
      data.maxDemand13M = extractFloatValue(line);
    }
    
    startIdx = endIdx + 1;
  }
  
  // TODO: Validate CRC16 checksum
  data.valid = true;
  
  return data;
}

// ============================================================================
// MQTT PUBLISHING
// ============================================================================

void publishToMQTT(const P1Data& data) {
  if (!mqttClient.connected() || mqttConfig.topic.length() == 0) {
    return;
  }
  
  // Create JSON document (matching Xenn format)
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
  
  mqttClient.publish(mqttConfig.topic.c_str(), jsonString.c_str());
  Serial.println("✅ Published to MQTT");
}

// ============================================================================
// WEBSOCKET BROADCASTING
// ============================================================================

void broadcastToWebSocket(const P1Data& data) {
  // Create JSON document (matching Xenn WebSocket format)
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
  Serial.println("📡 Broadcasted to WebSocket clients");
}

// ============================================================================
// P1 UART READER TASK
// ============================================================================

void p1ReaderTask(void* pvParameters) {
  Serial.println("   [P1 Task] Starting...");
  
  // Wait a bit before initializing Serial2
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  Serial.println("   [P1 Task] Initial delay complete");
  
  // Initialize Serial2 in the task, not in setup()
  // This avoids potential conflicts during setup()
  Serial.println("   [P1 Task] Initializing Serial2...");
  Serial.flush();
  
  Serial2.begin(P1_BAUD_RATE, SERIAL_8N1, P1_RX_PIN, P1_TX_PIN);
  Serial.println("   [P1 Task] Serial2.begin() completed");
  Serial.flush();
  
  Serial2.setRxBufferSize(P1_BUFFER_SIZE);
  Serial.println("   [P1 Task] Serial2.setRxBufferSize() completed");
  Serial.flush();
  
  delay(500);
  Serial.println("✅ P1 reader task started");
  
  // Use static buffer to avoid stack issues
  static char telegram[P1_BUFFER_SIZE];
  int telegramLen = 0;
  bool receiving = false;
  
  while (true) {
    // Always delay to prevent watchdog issues
    vTaskDelay(50 / portTICK_PERIOD_MS);
    
    if (Serial2.available() > 0) {
      char c = Serial2.read();
      
      // Start of telegram
      if (c == '/') {
        receiving = true;
        telegramLen = 0;
        telegram[0] = '/';
        telegram[1] = '\0';
        telegramLen = 1;
        state.meterConnected = true;
      }
      // Receiving telegram
      else if (receiving && telegramLen < P1_BUFFER_SIZE - 2) {
        telegram[telegramLen++] = c;
        telegram[telegramLen] = '\0';
        
        // End of telegram
        if (c == '!') {
          // Read CRC (next 4 characters)
          for (int i = 0; i < 4 && Serial2.available() && telegramLen < P1_BUFFER_SIZE - 2; i++) {
            telegram[telegramLen++] = Serial2.read();
            telegram[telegramLen] = '\0';
          }
          
          receiving = false;
          
          // Only log if we got a complete telegram
          if (telegramLen > 5) {
            Serial.println("📄 Telegram received (" + String(telegramLen) + " bytes)");
            
            // Parse telegram (create String only when needed)
            String telegramStr = String(telegram);
            P1Data data = parseTelegram(telegramStr);
            
            if (data.valid) {
              Serial.println("✅ Meter telegram ready");
              latestData = data;
              
              // Publish data
              publishToMQTT(data);
              broadcastToWebSocket(data);
            } else {
              Serial.println("❌ Telegram parsing failed");
            }
          }
          
          telegramLen = 0;
          telegram[0] = '\0';
        }
      }
    }
  }
}

// ============================================================================
// REST API HANDLERS
// ============================================================================

void setupAPI() {
  // GET /api/config
  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument doc;
    
    doc["email"] = preferences.getString("email", "");
    
    // Get IP address (either STA or AP)
    if (WiFi.status() == WL_CONNECTED) {
      doc["dongle_ip"] = WiFi.localIP().toString();
    } else {
      doc["dongle_ip"] = WiFi.softAPIP().toString();
    }
    
    JsonObject wifi = doc["wifi"].to<JsonObject>();
    if (WiFi.status() == WL_CONNECTED) {
      wifi["ssid"] = WiFi.SSID();
    } else {
      wifi["ssid"] = preferences.getString("wifi_ssid", "");
    }
    
    JsonObject mqtt = doc["mqtt"].to<JsonObject>();
    mqtt["host"] = mqttConfig.host;
    mqtt["port"] = mqttConfig.port;
    mqtt["topic"] = mqttConfig.topic;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  // GET /api/state
  server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument doc;
    
    // Check actual WiFi status
    doc["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
    doc["meter_connected"] = state.meterConnected;
    doc["cloud_connected"] = state.cloudConnected;
    doc["ap_mode"] = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA);
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  // GET /api/system
  server.on("/api/system", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument doc;
    
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["serial_number"] = state.serialNumber;
    doc["device_id"] = state.deviceId;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  // PATCH /api/config/wifi
  server.on("/api/config/wifi", HTTP_PATCH, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, (const char*)data);
      
      if (!error && doc.containsKey("wifi")) {
        JsonObject wifi = doc["wifi"];
        String ssid = wifi["ssid"] | "";
        String password = wifi["password"] | "";
        
        Serial.println("📝 Saving WiFi credentials:");
        Serial.println("   SSID: " + ssid);
        String pwdDisplay = password.length() > 0 ? "***" : "(empty)";
        Serial.println("   Password: " + pwdDisplay);
        
        if (ssid.length() > 0) {
          preferences.putString("wifi_ssid", ssid);
          preferences.putString("wifi_password", password);
          // Don't call preferences.end() here - it's opened in setup() and should stay open
          
          Serial.println("✅ WiFi credentials saved, restarting...");
          
          request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"WiFi credentials saved, device will restart\"}");
          
          // Give time for response to be sent
          delay(500);
          ESP.restart();
          return;
        } else {
          request->send(400, "application/json", "{\"error\":\"SSID cannot be empty\"}");
          return;
        }
      }
      
      request->send(400, "application/json", "{\"error\":\"Invalid request\"}");
    }
  );
  
  // PATCH /api/config/mqtt
  server.on("/api/config/mqtt", HTTP_PATCH, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, (const char*)data);
      
      if (!error && doc.containsKey("mqtt")) {
        JsonObject mqtt = doc["mqtt"];
        
        mqttConfig.host = mqtt["host"] | "";
        mqttConfig.port = mqtt["port"] | DEFAULT_MQTT_PORT;
        mqttConfig.topic = mqtt["topic"] | "";
        
        preferences.putString("mqtt_host", mqttConfig.host);
        preferences.putUShort("mqtt_port", mqttConfig.port);
        preferences.putString("mqtt_topic", mqttConfig.topic);
        
        request->send(200, "application/json", "{\"status\":\"ok\"}");
        
        // Reconnect MQTT with new settings
        if (mqttConfig.host.length() > 0) {
          mqttClient.setServer(mqttConfig.host.c_str(), mqttConfig.port);
        }
        return;
      }
      
      request->send(400, "application/json", "{\"error\":\"Invalid request\"}");
    }
  );
  
  // PATCH /api/system/reboot
  server.on("/api/system/reboot", HTTP_PATCH, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", "{\"status\":\"rebooting\"}");
    delay(1000);
    ESP.restart();
  });
  
  // PATCH /api/system/factory_reset
  server.on("/api/system/factory_reset", HTTP_PATCH, [](AsyncWebServerRequest *request){
    preferences.clear();
    request->send(200, "application/json", "{\"status\":\"reset\"}");
    delay(1000);
    ESP.restart();
  });
  
  // GET /api/knock (health check)
  server.on("/api/knock", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", "{\"status\":\"alive\"}");
  });
  
  // GET /api/config/wifiscan - Scan WiFi networks
  server.on("/api/config/wifiscan", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument doc;
    JsonArray networks = doc["networks"].to<JsonArray>();
    
    // Check if scan is already in progress
    int n = WiFi.scanComplete();
    
    if (n == -1) {
      // Scan in progress, return status
      doc["status"] = "scanning";
      String response;
      serializeJson(doc, response);
      request->send(200, "application/json", response);
      return;
    }
    
    // Start new scan (async, non-blocking)
    WiFi.scanNetworks(true, true);  // async=true, show_hidden=false
    
    // Wait a bit for scan to start
    delay(500);
    
    // Check scan status again
    n = WiFi.scanComplete();
    
    if (n == -1) {
      // Scan started, still in progress
      doc["status"] = "scanning";
    } else if (n == -2) {
      // Scan not started
      doc["status"] = "failed";
      doc["error"] = "Failed to start scan";
    } else if (n >= 0) {
      // Scan complete, return results
      doc["status"] = "complete";
      for (int i = 0; i < n; i++) {
        JsonObject net = networks.add<JsonObject>();
        net["ssid"] = WiFi.SSID(i);
        net["rssi"] = WiFi.RSSI(i);
        net["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open" : "encrypted";
      }
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  // Serve Web UI pages
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = getWebPage("/");
    request->send(200, "text/html", html);
  });
  
  server.on("/live", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = getWebPage("/live");
    request->send(200, "text/html", html);
  });
  
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = getWebPage("/settings");
    request->send(200, "text/html", html);
  });
  
  server.on("/system", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = getWebPage("/system");
    request->send(200, "text/html", html);
  });
  
  // Fallback for 404
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "Not found");
  });
}

// ============================================================================
// WEBSOCKET HANDLER
// ============================================================================

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("📱 WebSocket client connected: %u\n", client->id());
    Serial.println("ℹ️  Live meter connection established");
  }
  else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("📱 WebSocket client disconnected: %u\n", client->id());
    Serial.println("ℹ️  Live connection with meter closed");
  }
}

// ============================================================================
// WIFI & MQTT CONNECTION
// ============================================================================

void setupWiFi() {
  // Load saved WiFi credentials
  String savedSSID = preferences.getString("wifi_ssid", "");
  String savedPassword = preferences.getString("wifi_password", "");
  
  // Set AP credentials - SSID format: "OpenWatt-P1XXXXXX"
  String apSSID = String(AP_SSID_PREFIX) + state.deviceId.substring(2);
  String apPassword = generatePassword(state.deviceId);
  
  Serial.println("📶 Starting WiFi...");
  
  // Always start AP mode first (for configuration access)
  Serial.println("📶 Starting AP mode...");
  Serial.println("   AP SSID: " + apSSID);
  Serial.println("   AP Password: " + apPassword);
  
  WiFi.mode(WIFI_AP_STA);  // Enable both AP and STA modes
  WiFi.softAP(apSSID.c_str(), apPassword.c_str());
  
  // Set static IP for AP
  IPAddress localIP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(localIP, gateway, subnet);
  
  Serial.println("✅ AP mode started");
  Serial.println("   AP IP: " + WiFi.softAPIP().toString());
  
  // Try to connect to saved network if available
  if (savedSSID.length() > 0) {
    Serial.println("   Attempting to connect to: " + savedSSID);
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    
    // Wait for connection (max 15 seconds)
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
      delay(500);
      Serial.print(".");
      attempts++;
      yield();  // Feed watchdog
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
      state.wifiConnected = true;
      Serial.println("✅ WiFi connected!");
      Serial.println("   IP: " + WiFi.localIP().toString());
      Serial.println("   SSID: " + WiFi.SSID());
      Serial.println("   AP still available at: " + WiFi.softAPIP().toString());
    } else {
      Serial.println("❌ Failed to connect to saved network");
      Serial.println("   AP mode still available for configuration");
      state.wifiConnected = false;
    }
  } else {
    state.wifiConnected = false;
  }
}

void setupMQTT() {
  // Load MQTT config from NVS
  mqttConfig.host = preferences.getString("mqtt_host", "");
  mqttConfig.port = preferences.getUShort("mqtt_port", DEFAULT_MQTT_PORT);
  mqttConfig.topic = preferences.getString("mqtt_topic", "");
  
  if (mqttConfig.host.length() > 0) {
    mqttClient.setServer(mqttConfig.host.c_str(), mqttConfig.port);
    
    Serial.println("📡 Connecting to MQTT...");
    Serial.println("   Host: " + mqttConfig.host + ":" + String(mqttConfig.port));
    Serial.println("   Topic: " + mqttConfig.topic);
    
    if (mqttClient.connect(state.deviceId.c_str())) {
      Serial.println("✅ MQTT connected");
      state.cloudConnected = true;
    } else {
      Serial.println("❌ MQTT connection failed");
    }
  }
}

void reconnectMQTT() {
  if (!mqttClient.connected() && mqttConfig.host.length() > 0) {
    if (mqttClient.connect(state.deviceId.c_str())) {
      Serial.println("✅ MQTT reconnected");
      state.cloudConnected = true;
    }
  }
}

// ============================================================================
// UART SETUP (P1 Port)
// ============================================================================

void setupP1UART() {
  Serial.println("🔌 Initializing P1 UART...");
  Serial.println("   Port: UART2");
  Serial.println("   Baud: 115200");
  Serial.println("   RX Pin: " + String(P1_RX_PIN));
  
  // Don't initialize Serial2 here - let the task do it
  // This avoids potential conflicts during setup()
  
  // Start P1 reader task - don't pin to core, let scheduler decide
  Serial.println("   Creating task...");
  BaseType_t taskResult = xTaskCreate(
    p1ReaderTask,
    "P1_Reader",
    8192,  // Reduced but adequate stack size
    NULL,
    3,     // Lower priority to avoid blocking other tasks
    NULL
  );
  
  Serial.println("   Task creation returned: " + String(taskResult));
  
  if (taskResult == pdPASS) {
    Serial.println("✅ P1 reader task created");
    delay(100);  // Give task time to start
  } else {
    Serial.println("❌ Failed to create P1 reader task!");
  }
}

// ============================================================================
// SETUP & MAIN LOOP
// ============================================================================

void setup() {
  // Feed watchdog immediately
  yield();
  
  Serial.begin(115200);
  delay(1000);
  yield();
  
  Serial.println();
  yield();
  Serial.println("=================================");
  yield();
  Serial.println("OpenWatt P1 Reader");
  yield();
  Serial.println("=================================");
  yield();
  Serial.print("Firmware: ");
  Serial.println(FIRMWARE_VERSION);
  Serial.println();
  yield();
  delay(200);
  yield();
  
  Serial.println("Step 1: NVS init");
  yield();
  delay(100);
  yield();
  
  // Initialize NVS with error handling
  bool nvsOk = preferences.begin("openwatt", false);
  yield();
  
  if (!nvsOk) {
    Serial.println("ERROR: NVS init failed!");
    delay(1000);
    yield();
  }
  
  Serial.println("Step 2: NVS done");
  yield();
  delay(100);
  yield();
  
  Serial.println("Step 3: Get MAC");
  yield();
  delay(100);
  yield();
  
  // Generate device ID and serial number
  state.deviceId = getDeviceId();
  yield();
  
  Serial.println("Step 4: Device ID done");
  yield();
  delay(100);
  yield();
  
  state.serialNumber = getDeviceName();
  yield();
  
  Serial.println("Step 5: Device name done");
  yield();
  delay(100);
  yield();
  
  Serial.println("Device Info:");
  yield();
  Serial.print("   ID: ");
  Serial.println(state.deviceId);
  yield();
  Serial.print("   Serial: ");
  Serial.println(state.serialNumber);
  Serial.println();
  yield();
  delay(100);
  yield();
  
  // Setup WiFi
  setupWiFi();
  
  // Setup MQTT
  setupMQTT();
  
  // Setup WebSocket
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  
  // Setup REST API
  setupAPI();
  
  // Start HTTP server
  server.begin();
  Serial.println("✅ HTTP server started on port 80");
  
  // Setup P1 UART (disabled by default due to Serial2 initialization issues)
  #if ENABLE_P1_READER
    setupP1UART();
  #else
    Serial.println("⚠️  P1 UART disabled (ENABLE_P1_READER=0)");
  #endif
  
  Serial.println();
  Serial.println("🚀 *** Starting OpenWatt application ***");
  Serial.println("================================");
  Serial.println();
}

void loop() {
  // Feed watchdog
  yield();
  
  // Maintain MQTT connection
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();
  
  // Cleanup WebSocket
  ws.cleanupClients();
  
  delay(10);
}









#include "web_api.h"
#include "config.h"
#include "wifi_manager.h"
#include "serial_console.h"
#include "web_ui.h"
#include "ota_client.h"
#include <nvs.h>

#if ENABLE_MQTT
#include "mqtt_client.h"
#endif

#if ENABLE_OTA
#include "ota_update.h"
#endif

#define DEFAULT_MQTT_PORT 1883
#define NVS_NAMESPACE "openwatt"

static String getNvsString(const char* key) {
  nvs_handle_t h;
  if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) return "";
  size_t len = 0;
  if (nvs_get_str(h, key, NULL, &len) != ESP_OK) { nvs_close(h); return ""; }
  char* buf = (char*)malloc(len);
  if (!buf) { nvs_close(h); return ""; }
  esp_err_t err = nvs_get_str(h, key, buf, &len);
  nvs_close(h);
  if (err != ESP_OK) { free(buf); return ""; }
  String s(buf);
  free(buf);
  return s;
}

Preferences* WebAPI::prefs = nullptr;
String WebAPI::deviceId;
String WebAPI::serialNumber;
P1Data WebAPI::latestData;

void WebAPI::setup(AsyncWebServer& server, Preferences& prefs, const String& devId, const String& serial) {
  WebAPI::prefs = &prefs;
  WebAPI::deviceId = devId;
  WebAPI::serialNumber = serial;
  
  // GET /api/config
  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument doc;
    
    // Read values (keys should exist from initialization, so no errors)
    doc["email"] = getNvsString("email");
    
    if (WiFiManager::isConnected()) {
      doc["dongle_ip"] = WiFiManager::getIP();
    } else {
      doc["dongle_ip"] = WiFiManager::getAPIP();
    }
    
    JsonObject wifi = doc["wifi"].to<JsonObject>();
    if (WiFiManager::isConnected()) {
      wifi["ssid"] = WiFi.SSID();
    } else {
      wifi["ssid"] = WiFiManager::getSavedSSID(*WebAPI::prefs);
    }
    
    JsonObject mqtt = doc["mqtt"].to<JsonObject>();
    #if ENABLE_MQTT
    MQTTConfig mqttCfg = MQTTClient::getConfig();
    mqtt["host"] = mqttCfg.host;
    mqtt["port"] = mqttCfg.port;
    mqtt["topic"] = mqttCfg.topic;
    #else
    mqtt["host"] = "";
    mqtt["port"] = 1883;
    mqtt["topic"] = "";
    #endif
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  // GET /api/state
  server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument doc;
    
    doc["wifi_connected"] = WiFiManager::isConnected();
    doc["meter_connected"] = P1Reader::isConnected();
    #if ENABLE_MQTT
    doc["cloud_connected"] = MQTTClient::isConnected();
    #else
    doc["cloud_connected"] = false;
    #endif
    doc["ap_mode"] = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA);
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  // GET /api/system
  server.on("/api/system", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument doc;
    
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["serial_number"] = WebAPI::serialNumber;
    doc["device_id"] = WebAPI::deviceId;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // GET /api/meter - latest DSMR P1 reading (JSON, poll when WebSocket not used)
  server.on("/api/meter", HTTP_GET, [](AsyncWebServerRequest *request){
    const P1Data& d = WebAPI::latestData;
    JsonDocument doc;
    doc["meter_connected"] = P1Reader::isConnected();
    doc["valid"] = d.valid;
    doc["equipment_id"] = d.equipmentId;
    doc["timestamp"] = d.timestamp;
    doc["tariff"] = d.tariffIndicator;
    JsonObject consumption = doc["consumption"].to<JsonObject>();
    consumption["tariff1"] = d.consumptionT1;
    consumption["tariff2"] = d.consumptionT2;
    JsonObject production = doc["production"].to<JsonObject>();
    production["tariff1"] = d.productionT1;
    production["tariff2"] = d.productionT2;
    JsonObject power = doc["power"].to<JsonObject>();
    power["consumed_kw"] = d.powerConsumed;
    power["produced_kw"] = d.powerProduced;
    JsonObject demand = doc["demand"].to<JsonObject>();
    demand["current_kw"] = d.avgDemand;
    demand["max_month_kw"] = d.maxDemandMonth;
    demand["max_13m_kw"] = d.maxDemand13M;
    // OBIS-style keys (same as WebSocket) for compatibility
    doc["0-0:96.1.1"] = d.equipmentId;
    doc["0-0:1.0.0"] = d.timestamp;
    doc["1-0:1.8.1"] = d.consumptionT1;
    doc["1-0:2.8.1"] = d.productionT1;
    doc["1-0:1.8.2"] = d.consumptionT2;
    doc["1-0:2.8.2"] = d.productionT2;
    doc["1-0:1.7.0"] = d.powerConsumed;
    doc["1-0:2.7.0"] = d.powerProduced;
    doc["1-0:1.4.0"] = d.avgDemand;
    doc["1-0:1.6.0"] = d.maxDemandMonth;
    doc["0-0:98.1.0"] = d.maxDemand13M;
    // Include raw telegram for custom parsing
    doc["raw"] = P1Reader::getLastRawTelegram();
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  // GET /api/v1/data - Home Assistant compatible energy data endpoint
  server.on("/api/v1/data", HTTP_GET, [](AsyncWebServerRequest *request){
    const P1Data& d = WebAPI::latestData;
    JsonDocument doc;
    
    // WiFi information
    if (WiFiManager::isConnected()) {
      doc["wifi_ssid"] = WiFi.SSID();
      // Convert RSSI to percentage (approximate)
      int rssi = WiFi.RSSI();
      int strength = 0;
      if (rssi >= -50) strength = 100;
      else if (rssi <= -100) strength = 0;
      else strength = 2 * (rssi + 100);
      doc["wifi_strength"] = strength;
    } else {
      doc["wifi_ssid"] = "";
      doc["wifi_strength"] = 0;
    }
    
    // SMR version - derive from meter model if available
    if (d.meterModel.indexOf("SMR") >= 0 || d.meterModel.indexOf("50") >= 0) {
      doc["smr_version"] = 50;
    } else {
      doc["smr_version"] = 40;  // Default to 4.0 if unknown
    }
    
    // Meter information
    doc["meter_model"] = d.meterModel;
    doc["unique_id"] = d.equipmentId;
    
    // Tariff
    doc["active_tariff"] = d.tariffIndicator;
    
    // Energy totals (kWh)
    doc["total_power_import_kwh"] = d.consumptionT1 + d.consumptionT2 + d.consumptionT3 + d.consumptionT4 + d.consumptionT5;
    doc["total_power_import_t1_kwh"] = d.consumptionT1;
    doc["total_power_import_t2_kwh"] = d.consumptionT2;
    doc["total_power_export_kwh"] = d.productionT1 + d.productionT2 + d.productionT3 + d.productionT4 + d.productionT5;
    doc["total_power_export_t1_kwh"] = d.productionT1;
    doc["total_power_export_t2_kwh"] = d.productionT2;
    
    // Active power (convert kW to W)
    doc["active_power_w"] = d.powerConsumed * 1000;
    doc["active_power_l1_w"] = d.powerImportL1 * 1000;
    doc["active_power_l2_w"] = d.powerImportL2 * 1000;
    doc["active_power_l3_w"] = d.powerImportL3 * 1000;
    
    // Voltage and current
    doc["active_voltage_l1_v"] = d.voltageL1;
    doc["active_voltage_l2_v"] = d.voltageL2;
    doc["active_voltage_l3_v"] = d.voltageL3;
    doc["active_current_l1_a"] = d.currentL1;
    doc["active_current_l2_a"] = d.currentL2;
    doc["active_current_l3_a"] = d.currentL3;
    
    // Average and peak power
    doc["active_power_average_w"] = d.avgDemand * 1000;
    doc["montly_power_peak_w"] = d.maxDemandMonth * 1000;
    
    // Parse monthly peak timestamp from DSMR format (YYMMDDhhmmss) to timestamp
    if (d.maxDemandTimestamp.length() >= 12) {
      // Convert YYMMDDhhmmss to YYYYMMDDhhmmss format
      String ts = d.maxDemandTimestamp;
      if (ts.length() == 12) {  // YYMMDDhhmmss
        int year = ts.substring(0, 2).toInt();
        year += (year >= 50) ? 1900 : 2000;  // Assume 50+ = 1900s, <50 = 2000s
        ts = String(year) + ts.substring(2);
      }
      doc["montly_power_peak_timestamp"] = ts;
    } else {
      doc["montly_power_peak_timestamp"] = "";
    }
    
    // External devices array (empty for now)
    JsonArray external = doc["external"].to<JsonArray>();
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  // GET /api/meter/raw - Raw P1 telegram as plain text
  server.on("/api/meter/raw", HTTP_GET, [](AsyncWebServerRequest *request){
    String raw = P1Reader::getLastRawTelegram();
    if (raw.length() == 0) {
      request->send(404, "text/plain", "No telegram received yet");
    } else {
      request->send(200, "text/plain", raw);
    }
  });
  
  // PATCH /api/config/wifi - body can arrive in chunks, so accumulate then parse once
  static String wifiPatchBody;
  server.on("/api/config/wifi", HTTP_PATCH, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      if (index == 0) wifiPatchBody = "";
      if (data && len) wifiPatchBody.concat((const char*)data, len);
      if (index + len < total) return;  // more chunks to come

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, wifiPatchBody);
      if (error || !doc["wifi"].is<JsonObject>()) {
        request->send(400, "application/json", "{\"error\":\"Invalid request\"}");
        return;
      }
      JsonObject wifi = doc["wifi"];
      char ssidBuf[64];
      char pwdBuf[64];
      const char* sp = wifi["ssid"].as<const char*>();
      const char* pp = wifi["password"].as<const char*>();
      if (sp) { strncpy(ssidBuf, sp, sizeof(ssidBuf) - 1); ssidBuf[sizeof(ssidBuf) - 1] = '\0'; } else { ssidBuf[0] = '\0'; }
      if (pp) { strncpy(pwdBuf, pp, sizeof(pwdBuf) - 1); pwdBuf[sizeof(pwdBuf) - 1] = '\0'; } else { pwdBuf[0] = '\0'; }
      String ssidCopy(ssidBuf);
      String pwdCopy(pwdBuf);
      SerialConsole::println("Saving WiFi credentials:");
      SerialConsole::println("  SSID: " + ssidCopy);
      if (ssidCopy.length() == 0) {
        request->send(400, "application/json", "{\"error\":\"SSID cannot be empty\"}");
        return;
      }
      WiFiManager::saveCredentials(*WebAPI::prefs, ssidBuf, pwdBuf);
      request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"WiFi credentials saved, device will restart\"}");
      delay(2000);
      SerialConsole::println("Restarting...");
      ESP.restart();
    }
  );
  
  // PATCH /api/config/mqtt
  server.on("/api/config/mqtt", HTTP_PATCH, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      #if ENABLE_MQTT
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, (const char*)data);
      
      if (!error && doc["mqtt"].is<JsonObject>()) {
        JsonObject mqtt = doc["mqtt"];
        
        MQTTConfig config;
        config.host = mqtt["host"] | "";
        config.port = mqtt["port"] | DEFAULT_MQTT_PORT;
        config.topic = mqtt["topic"] | "";
        
        MQTTClient::setConfig(config);
        MQTTClient::saveConfig(*WebAPI::prefs);
        
        request->send(200, "application/json", "{\"status\":\"ok\"}");
        return;
      }
      
      request->send(400, "application/json", "{\"error\":\"Invalid request\"}");
      #else
      request->send(503, "application/json", "{\"error\":\"MQTT is disabled\"}");
      #endif
    }
  );
  
  // GET /api/config/wifiscan - synchronous scan with proper STA initialization
  server.on("/api/config/wifiscan", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument doc;
    JsonArray networks = doc["networks"].to<JsonArray>();

    // Ensure we're in AP_STA mode and STA is ready
    if (WiFi.getMode() != WIFI_AP_STA) {
      WiFi.mode(WIFI_AP_STA);
      delay(500); // Give STA time to initialize
    }
    
    // Wait for STA to be ready (max 3 seconds)
    int waitCount = 0;
    while (WiFi.status() == WL_NO_SHIELD && waitCount < 30) {
      delay(100);
      waitCount++;
    }
    
    yield();
    
    // Perform synchronous scan
    int n = WiFi.scanNetworks(false, false);  // sync, no hidden
    SerialConsole::println("WiFi scan: raw_count=" + String(n) + " (mode=" + String(WiFi.getMode()) + ")");
    
    if (n == -1) {
      doc["error"] = "Scan in progress";
      doc["status"] = "failed";
    } else if (n == -2) {
      doc["error"] = "Scan failed";
      doc["status"] = "failed";
    } else if (n >= 0) {
      doc["status"] = "complete";
      for (int i = 0; i < n; i++) {
        String ssid = WiFi.SSID(i);
        if (ssid.length() == 0) continue;
        if (i < 5) SerialConsole::println("  [" + String(i) + "] " + ssid + " " + String(WiFi.RSSI(i)) + " dBm");
        JsonObject net = networks.add<JsonObject>();
        net["ssid"] = ssid;
        net["rssi"] = WiFi.RSSI(i);
        net["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open" : "encrypted";
      }
      SerialConsole::println("  -> returned " + String(networks.size()) + " networks in JSON");
    }
    
    WiFi.scanDelete(); // Clean up scan results
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // GET /api/debug/nvs - probe WiFi NVS (namespace ow_wifi, keys ssid/pass). Use after save to see if blob is stored.
  server.on("/api/debug/nvs", HTTP_GET, [](AsyncWebServerRequest *request){
    nvs_handle_t h;
    esp_err_t err = nvs_open("ow_wifi", NVS_READONLY, &h);
    JsonDocument doc;
    doc["nvs_open_ow_wifi"] = (int)err;
    if (err != ESP_OK) {
      String r; serializeJson(doc, r);
      request->send(200, "application/json", r);
      return;
    }
    size_t len = 0;
    esp_err_t blobErr = nvs_get_blob(h, "ssid", NULL, &len);
    doc["ssid_blob_err"] = (int)blobErr;
    doc["ssid_blob_len"] = (int)len;
    len = 0;
    esp_err_t passErr = nvs_get_blob(h, "pass", NULL, &len);
    doc["pass_blob_err"] = (int)passErr;
    doc["pass_blob_len"] = (int)len;
    nvs_close(h);
    String r; serializeJson(doc, r);
    request->send(200, "application/json", r);
  });
  
  // PATCH /api/system/reboot
  server.on("/api/system/reboot", HTTP_PATCH, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", "{\"status\":\"rebooting\"}");
    delay(1000);
    ESP.restart();
  });

  // PATCH /api/system/bootloader - try to enter serial bootloader (GPIO0 low + reset). Run esptool right after.
  server.on("/api/system/bootloader", HTTP_PATCH, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", "{\"status\":\"entering_bootloader\",\"message\":\"GPIO0 low + reset. Run esptool within a few seconds.\"}");
    delay(500);
    const int bootPin = 0;  // GPIO0 = BOOT on most ESP32 boards
    pinMode(bootPin, OUTPUT);
    digitalWrite(bootPin, LOW);
    delay(200);
    ESP.restart();
  });
  
  // POST /api/system/ota-pull - download and flash from URL. Query: url= (e.g. ?url=http://192.0.2.1:28214/firmware.bin)
  server.on("/api/system/ota-pull", HTTP_POST, [](AsyncWebServerRequest *request){
    String url = request->hasParam("url", true) ? request->getParam("url", true)->value() : "";
    if (url.length() == 0) {
      request->send(400, "application/json", "{\"error\":\"missing url (query: url=...)\"}");
      return;
    }
    request->send(202, "application/json", "{\"status\":\"downloading\",\"url\":\"" + url + "\"}");
    OTAClient::downloadAndApplyFromURL(url);
  });

  // PATCH /api/system/check-update
  #if ENABLE_OTA
  server.on("/api/system/check-update", HTTP_PATCH, [](AsyncWebServerRequest *request){
    JsonDocument doc;
    doc["status"] = "checking";
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
    
    // Trigger OTA check (non-blocking, will reboot if update found)
    OTAUpdate::checkUpdate();
  });
  #else
  server.on("/api/system/check-update", HTTP_PATCH, [](AsyncWebServerRequest *request){
    JsonDocument doc;
    doc["status"] = "disabled";
    doc["message"] = "OTA updates are disabled";
    String response;
    serializeJson(doc, response);
    request->send(503, "application/json", response);
  });
  #endif
  
  // PATCH /api/system/factory_reset
  server.on("/api/system/factory_reset", HTTP_PATCH, [](AsyncWebServerRequest *request){
    WebAPI::prefs->clear();
    request->send(200, "application/json", "{\"status\":\"reset\"}");
    delay(1000);
    ESP.restart();
  });
  
  // GET /api/knock
  server.on("/api/knock", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", "{\"status\":\"alive\"}");
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
  
  // GET /api/debug/p1 - Debug P1 serial statistics
  server.on("/api/debug/p1", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument doc;
    doc["serial_port"] = "Serial1";
    doc["rx_pin"] = 21;
    doc["tx_pin"] = 22;
    doc["trigger_pin"] = 25;
    doc["baud_rate"] = 115200;
    doc["bytes_received"] = P1Reader::getBytesReceived();
    doc["telegram_count"] = P1Reader::getTelegramCount();
    doc["meter_connected"] = P1Reader::isConnected();
    doc["last_telegram"] = P1Reader::getLastRawTelegram();
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "Not found");
  });
}

void WebAPI::setLatestData(const P1Data& data) {
  latestData = data;
}

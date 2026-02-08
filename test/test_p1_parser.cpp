#include <unity.h>
#include <Arduino.h>
#include "p1_reader.h"

// Note: These tests require Arduino String class
// For native platform testing, we may need mocks or use ESP32 test environment

// Sample valid P1 telegram (from reverse engineering)
const char* VALID_TELEGRAM = 
  "/XMX5LGBBFG1009325509\r\n"
  "\r\n"
  "1-0:0.2.8(50)\r\n"
  "0-0:1.0.0(250116120000W)\r\n"
  "0-0:96.1.1(4530303531303035333030303035333137)\r\n"
  "1-0:1.8.1(001234.567*kWh)\r\n"
  "1-0:1.8.2(005678.901*kWh)\r\n"
  "1-0:2.8.1(000000.000*kWh)\r\n"
  "1-0:2.8.2(000000.000*kWh)\r\n"
  "0-0:96.14.0(0001)\r\n"
  "1-0:1.7.0(00.123*kW)\r\n"
  "1-0:2.7.0(00.000*kW)\r\n"
  "0-0:96.7.21(00015)\r\n"
  "0-0:96.7.9(00001)\r\n"
  "1-0:99.97.0(0)(0-0:96.7.19)\r\n"
  "1-0:32.32.0(00000)\r\n"
  "1-0:32.36.0(00000)\r\n"
  "0-0:96.13.1()\r\n"
  "0-0:96.13.0()\r\n"
  "1-0:1.4.0(00.010*kW)\r\n"
  "1-0:1.6.0(00.010*kW)(250116170000W)\r\n"
  "0-0:98.1.0(00.020*kW)\r\n"
  "!A1B2\r\n";

// Telegram without CRC (invalid)
const char* INVALID_TELEGRAM_NO_CRC = 
  "/XMX5LGBBFG1009325509\r\n"
  "1-0:1.8.1(001234.567*kWh)\r\n";

// Telegram with wrong start character
const char* INVALID_TELEGRAM_WRONG_START = 
  "XMX5LGBBFG1009325509\r\n"
  "1-0:1.8.1(001234.567*kWh)\r\n"
  "!A1B2\r\n";

// Empty telegram
const char* EMPTY_TELEGRAM = "";

void setUp(void) __attribute__((weak));
void tearDown(void) __attribute__((weak));

void setUp(void) {
  // Set up test fixtures
}

void tearDown(void) {
  // Clean up after tests
}

void test_parse_valid_telegram(void) {
  P1Data data = P1Reader::parseTelegram(String(VALID_TELEGRAM));
  
  TEST_ASSERT_TRUE(data.valid);
  TEST_ASSERT_EQUAL_STRING("4530303531303035333030303035333137", data.equipmentId.c_str());
  TEST_ASSERT_EQUAL_STRING("250116120000W", data.timestamp.c_str());
  TEST_ASSERT_EQUAL(1, data.tariffIndicator);
  TEST_ASSERT_FLOAT_WITHIN(0.001, 1234.567, data.consumptionT1);
  TEST_ASSERT_FLOAT_WITHIN(0.001, 5678.901, data.consumptionT2);
  TEST_ASSERT_FLOAT_WITHIN(0.001, 0.0, data.productionT1);
  TEST_ASSERT_FLOAT_WITHIN(0.001, 0.0, data.productionT2);
  TEST_ASSERT_FLOAT_WITHIN(0.001, 0.123, data.powerConsumed);
  TEST_ASSERT_FLOAT_WITHIN(0.001, 0.0, data.powerProduced);
  TEST_ASSERT_FLOAT_WITHIN(0.001, 0.010, data.avgDemand);
  TEST_ASSERT_FLOAT_WITHIN(0.001, 0.010, data.maxDemandMonth);
  TEST_ASSERT_FLOAT_WITHIN(0.001, 0.020, data.maxDemand13M);
}

void test_parse_invalid_telegram_no_crc(void) {
  P1Data data = P1Reader::parseTelegram(String(INVALID_TELEGRAM_NO_CRC));
  
  // Should fail CRC validation
  TEST_ASSERT_FALSE(data.valid);
}

void test_parse_invalid_telegram_wrong_start(void) {
  P1Data data = P1Reader::parseTelegram(String(INVALID_TELEGRAM_WRONG_START));
  
  // Should fail because doesn't start with '/'
  TEST_ASSERT_FALSE(data.valid);
}

void test_parse_empty_telegram(void) {
  P1Data data = P1Reader::parseTelegram(String(EMPTY_TELEGRAM));
  
  TEST_ASSERT_FALSE(data.valid);
}

// Test functions - main() is in test_main.cpp

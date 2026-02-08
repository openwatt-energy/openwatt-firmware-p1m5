#include <unity.h>
#include <Arduino.h>
#include <string.h>
#include "mqtt_auth.h"

// Note: These tests require Arduino String class and mbedTLS
// For native platform testing, we may need mocks or use ESP32 test environment

// Test vectors - these should match Python implementation
// Python: generate_password("P1846680", "test_secret_key")
// We'll test with known values

void setUp(void) __attribute__((weak));
void tearDown(void) __attribute__((weak));

void setUp(void) {
  // Set up test fixtures
}

void tearDown(void) {
  // Clean up after tests
}

void test_generate_password_basic(void) {
  String deviceId = "P1846680";
  String secretKey = "test_secret_key";
  
  String password = MQTTAuth::generatePassword(deviceId, secretKey);
  
  // Password should be base64 encoded (14-15 chars typically for 10 bytes)
  TEST_ASSERT_TRUE(password.length() > 0);
  TEST_ASSERT_TRUE(password.length() <= 20);  // Base64 of 10 bytes = 14 chars (with padding)
  
  // Should only contain base64 characters
  for (size_t i = 0; i < password.length(); i++) {
    char c = password.charAt(i);
    bool isBase64 = (c >= 'A' && c <= 'Z') || 
                    (c >= 'a' && c <= 'z') || 
                    (c >= '0' && c <= '9') || 
                    c == '+' || c == '/' || c == '=';
    TEST_ASSERT_TRUE(isBase64);
  }
}

void test_generate_password_different_devices(void) {
  String secretKey = "test_secret_key";
  
  String password1 = MQTTAuth::generatePassword("P1846680", secretKey);
  String password2 = MQTTAuth::generatePassword("P1846681", secretKey);
  
  // Different devices should have different passwords
  TEST_ASSERT_FALSE(password1 == password2);
  TEST_ASSERT_NOT_EQUAL(0, strcmp(password1.c_str(), password2.c_str()));
}

void test_generate_password_different_secrets(void) {
  String deviceId = "P1846680";
  
  String password1 = MQTTAuth::generatePassword(deviceId, "secret1");
  String password2 = MQTTAuth::generatePassword(deviceId, "secret2");
  
  // Different secrets should produce different passwords
  TEST_ASSERT_FALSE(password1 == password2);
  TEST_ASSERT_NOT_EQUAL(0, strcmp(password1.c_str(), password2.c_str()));
}

void test_generate_password_empty_inputs(void) {
  // Empty device ID should still produce a password (though invalid)
  String password1 = MQTTAuth::generatePassword("", "secret");
  TEST_ASSERT_TRUE(password1.length() > 0);
  
  // Empty secret should still produce a password
  String password2 = MQTTAuth::generatePassword("P1846680", "");
  TEST_ASSERT_TRUE(password2.length() > 0);
}

void test_generate_password_consistency(void) {
  String deviceId = "P1846680";
  String secretKey = "test_secret_key";
  
  // Same inputs should produce same output
  String password1 = MQTTAuth::generatePassword(deviceId, secretKey);
  String password2 = MQTTAuth::generatePassword(deviceId, secretKey);
  
  TEST_ASSERT_EQUAL_STRING(password1.c_str(), password2.c_str());
}

// Test functions - main() is in test_main.cpp

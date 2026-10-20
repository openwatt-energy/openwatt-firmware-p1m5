#include <unity.h>
#include <Arduino.h>

// Arduino framework requires setup() and loop()
void setup() {
  // Initialize serial for test output
  Serial.begin(115200);
  delay(2000);
  
  UNITY_BEGIN();
  
  // Run P1 parser tests
  extern void test_parse_valid_telegram(void);
  extern void test_parse_invalid_telegram_no_crc(void);
  extern void test_parse_invalid_telegram_wrong_start(void);
  extern void test_parse_empty_telegram(void);
  
  RUN_TEST(test_parse_valid_telegram);
  RUN_TEST(test_parse_invalid_telegram_no_crc);
  RUN_TEST(test_parse_invalid_telegram_wrong_start);
  RUN_TEST(test_parse_empty_telegram);
  
  // Run MQTT auth tests
  extern void test_generate_password_basic(void);
  extern void test_generate_password_different_devices(void);
  extern void test_generate_password_different_secrets(void);
  extern void test_generate_password_empty_inputs(void);
  extern void test_generate_password_consistency(void);
  
  RUN_TEST(test_generate_password_basic);
  RUN_TEST(test_generate_password_different_devices);
  RUN_TEST(test_generate_password_different_secrets);
  RUN_TEST(test_generate_password_empty_inputs);
  RUN_TEST(test_generate_password_consistency);
  
  UNITY_END();
}

void loop() {
  // Tests run once in setup()
  delay(1000);
}

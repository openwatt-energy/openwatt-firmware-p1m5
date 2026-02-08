#include "mqtt_auth.h"
#include "mbedtls/sha256.h"
#include "mbedtls/base64.h"
#include <string.h>

String MQTTAuth::generatePassword(const String& deviceId, const String& secretKey) {
  // Concatenate secret_key + device_id (matching Python: "%s%s" % (secret_key, p1_number))
  String combined = secretKey + deviceId;
  
  // SHA256 hash using mbedTLS (built into ESP32)
  uint8_t hash[32];
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);  // 0 = SHA256 (not SHA224)
  mbedtls_sha256_update(&ctx, (const unsigned char*)combined.c_str(), combined.length());
  mbedtls_sha256_finish(&ctx, hash);
  mbedtls_sha256_free(&ctx);
  
  // Take first 10 bytes
  uint8_t hash_10_bytes[10];
  memcpy(hash_10_bytes, hash, 10);
  
  // Base64 encode using mbedTLS (built into ESP32)
  return base64Encode(hash_10_bytes, 10);
}

String MQTTAuth::base64Encode(const uint8_t* data, size_t length) {
  // Use mbedTLS base64 encoding (built into ESP32)
  size_t output_len = 0;
  unsigned char* encoded = nullptr;
  
  // Calculate output length
  mbedtls_base64_encode(nullptr, 0, &output_len, data, length);
  
  // Allocate buffer
  encoded = (unsigned char*)malloc(output_len + 1);
  if (!encoded) {
    return String("");
  }
  
  // Encode
  size_t actual_output_len = 0;
  int ret = mbedtls_base64_encode(encoded, output_len, &actual_output_len, data, length);
  
  if (ret != 0) {
    free(encoded);
    return String("");
  }
  
  encoded[actual_output_len] = '\0';
  String result = String((char*)encoded);
  free(encoded);
  
  return result;
}

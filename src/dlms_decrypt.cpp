#include "dlms_decrypt.h"
#include "serial_console.h"
#include <mbedtls/gcm.h>
#include <nvs.h>

// Luxembourg Smarty meter decryption (Creos/Luxmetering):
// - Uses 17-byte fixed AAD: 0x30 + 00112233445566778899AABBCCDDEEFF
// - Frame structure: DB (1) + Length (1) + SystemTitle (8) + 82045030 (4) + FrameCounter (4) + Ciphertext + Tag (12)

bool DLMSDecrypt::hexToBinary(const String& hexStr, uint8_t* output) {
  if (hexStr.length() % 2 != 0) {
    return false;  // Hex string must have even length
  }

  for (size_t i = 0; i < hexStr.length(); i += 2) {
    char highNibble = hexStr.charAt(i);
    char lowNibble = hexStr.charAt(i + 1);

    uint8_t high = 0, low = 0;

    // Convert high nibble
    if (highNibble >= '0' && highNibble <= '9') {
      high = highNibble - '0';
    } else if (highNibble >= 'A' && highNibble <= 'F') {
      high = highNibble - 'A' + 10;
    } else if (highNibble >= 'a' && highNibble <= 'f') {
      high = highNibble - 'a' + 10;
    } else {
      return false;
    }

    // Convert low nibble
    if (lowNibble >= '0' && lowNibble <= '9') {
      low = lowNibble - '0';
    } else if (lowNibble >= 'A' && lowNibble <= 'F') {
      low = lowNibble - 'A' + 10;
    } else if (lowNibble >= 'a' && lowNibble <= 'f') {
      low = lowNibble - 'a' + 10;
    } else {
      return false;
    }

    output[i / 2] = (high << 4) | low;
  }

  return true;
}

bool DLMSDecrypt::isEncrypted(const uint8_t* data, size_t len) {
  // Check if frame starts with 0xDB (DLMS GBT wrapper with ciphering)
  return (len > 0 && data[0] == 0xDB);
}

bool DLMSDecrypt::decrypt(
  const uint8_t* encryptedFrame,
  size_t frameLength,
  const String& decryptionKeyHex,
  uint8_t* plaintextOut,
  size_t maxPlaintextLen,
  size_t* plaintextLen
) {
  // Validate input
  if (!encryptedFrame || frameLength < 30 || !plaintextOut || !plaintextLen) {
    SerialConsole::println("[DLMS] Error: Invalid input parameters");
    return false;
  }

  // Verify frame starts with 0xDB
  if (encryptedFrame[0] != 0xDB) {
    SerialConsole::println("[DLMS] Error: Frame doesn't start with 0xDB");
    return false;
  }

  // Verify System Title length byte
  if (encryptedFrame[1] != 0x08) {
    SerialConsole::println("[DLMS] Error: System Title length is not 0x08");
    return false;
  }

  // Extract System Title (8 bytes at offset 2)
  const uint8_t* systemTitle = &encryptedFrame[2];

  // Extract Frame Counter (4 bytes at offset 14-17)
  // Note: Bytes 10-13 contain 82 04 50 30 (BER length encoding + security from spec)
  // The actual frame counter used in IV is at bytes 14-17
  const uint8_t* frameCounter = &encryptedFrame[14];
  uint32_t frameCounterValue = (frameCounter[0] << 24) | (frameCounter[1] << 16) |
                                (frameCounter[2] << 8) | frameCounter[3];

  // Ciphertext starts at offset 18
  const uint8_t* ciphertext = &encryptedFrame[18];
  size_t ciphertextLength = frameLength - 18;

  // GCM tag is the last 12 bytes of ciphertext
  if (ciphertextLength < 12) {
    SerialConsole::println("[DLMS] Error: Ciphertext too short for GCM tag");
    return false;
  }

  size_t actualCiphertextLen = ciphertextLength - 12;
  const uint8_t* tag = &ciphertext[actualCiphertextLen];

  // Check output buffer size
  if (maxPlaintextLen < actualCiphertextLen) {
    SerialConsole::println("[DLMS] Error: Output buffer too small");
    return false;
  }

  // Construct 12-byte IV (nonce) = System Title (8) + Frame Counter (4)
  uint8_t iv[12];
  memcpy(iv, systemTitle, 8);
  memcpy(&iv[8], frameCounter, 4);

  // Convert decryption key from hex to binary
  if (decryptionKeyHex.length() != 32) {
    SerialConsole::println("[DLMS] Error: Key must be 32 hex characters");
    return false;
  }

  uint8_t key[16];
  if (!hexToBinary(decryptionKeyHex, key)) {
    SerialConsole::println("[DLMS] Error: Failed to convert key from hex");
    return false;
  }

  // Debug logging
  SerialConsole::println("[DLMS] Decryption parameters:");
  SerialConsole::print("[DLMS]   System Title: ");
  for (int i = 0; i < 8; i++) {
    char buf[3];
    sprintf(buf, "%02X", systemTitle[i]);
    SerialConsole::print(String(buf));
  }
  SerialConsole::println("");

  SerialConsole::println("[DLMS]   Frame Counter: " + String(frameCounterValue));
  SerialConsole::println("[DLMS]   Ciphertext length: " + String(actualCiphertextLen));

  // Initialize mbedtls GCM context
  mbedtls_gcm_context gcm;
  mbedtls_gcm_init(&gcm);

  // Set key
  int ret = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 128);
  if (ret != 0) {
    SerialConsole::println("[DLMS] Error: mbedtls_gcm_setkey failed: " + String(ret));
    mbedtls_gcm_free(&gcm);
    return false;
  }

  // Luxembourg Luxmetering AAD: Security byte (0x30) + 16-byte fixed authentication key
  const uint8_t aad[17] = {
    0x30, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
    0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
  };

  // Decrypt and verify
  ret = mbedtls_gcm_auth_decrypt(
    &gcm,
    actualCiphertextLen,  // Length of ciphertext (excluding tag)
    iv,                    // 12-byte nonce
    12,                    // Nonce length
    aad,                   // 17-byte Luxembourg fixed AAD
    17,                    // AAD length
    tag,                   // 12-byte authentication tag
    12,                    // Tag length
    ciphertext,            // Input ciphertext
    plaintextOut           // Output plaintext
  );

  mbedtls_gcm_free(&gcm);

  if (ret != 0) {
    SerialConsole::println("[DLMS] Decryption failed (wrong key or corrupted data): " + String(ret));
    return false;
  }

  *plaintextLen = actualCiphertextLen;
  SerialConsole::println("[DLMS] ✅ Decryption successful! Plaintext length: " + String(*plaintextLen));

  return true;
}

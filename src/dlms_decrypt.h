#ifndef DLMS_DECRYPT_H
#define DLMS_DECRYPT_H

#include <Arduino.h>

/**
 * DLMS AES-128-GCM Decryption Module
 *
 * Implements decryption for encrypted P1 telegrams from Luxembourg meters (Creos/Luxmetering).
 * Based on the E-Meter P1 specification v20210308.
 *
 * Frame structure:
 *   Byte 0:       0xDB (GBT wrapper with ciphering)
 *   Byte 1:       0x08 (System Title length)
 *   Byte 2-9:     System Title (8 bytes)
 *   Byte 10:      0x82 (BER length encoding)
 *   Byte 11-12:   Data length (2 bytes, big-endian)
 *   Byte 13:      Security Control byte (0x30 = authenticated + encrypted)
 *   Byte 14-17:   Frame Counter (4 bytes, big-endian)
 *   Byte 18+:     Ciphertext + 12-byte GCM authentication tag
 *
 * Decryption:
 *   - IV (nonce) = System Title (8 bytes) + Frame Counter (4 bytes) = 12 bytes
 *   - AAD = Fixed 17 bytes: 3000112233445566778899AABBCCDDEEFF
 *   - Algorithm: AES-128-GCM with 16-byte key
 */

class DLMSDecrypt {
public:
  /**
   * Decrypt a DLMS encrypted frame
   *
   * @param encryptedFrame Raw binary frame starting with 0xDB
   * @param frameLength Length of encrypted frame in bytes
   * @param decryptionKey 16-byte AES key (pass as 32-char hex string will be converted internally)
   * @param plaintextOut Buffer to store decrypted plaintext (caller must allocate)
   * @param maxPlaintextLen Maximum size of plaintextOut buffer
   * @param plaintextLen Output: actual length of decrypted plaintext
   * @return true if decryption succeeded, false otherwise
   */
  static bool decrypt(
    const uint8_t* encryptedFrame,
    size_t frameLength,
    const String& decryptionKeyHex,
    uint8_t* plaintextOut,
    size_t maxPlaintextLen,
    size_t* plaintextLen
  );

  /**
   * Check if a frame is an encrypted DLMS frame
   * @param data First few bytes of frame
   * @param len Length of data
   * @return true if frame starts with 0xDB (encrypted), false otherwise
   */
  static bool isEncrypted(const uint8_t* data, size_t len);

private:
  /**
   * Convert hex string to binary
   * @param hexStr Hex string (e.g., "21C4D799...")
   * @param output Output buffer (must be at least hexStr.length()/2 bytes)
   * @return true if conversion succeeded
   */
  static bool hexToBinary(const String& hexStr, uint8_t* output);
};

#endif

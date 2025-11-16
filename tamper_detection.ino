#include "FS.h"
#include "SD.h"
#include <SPI.h>
#include <mbedtls/aes.h>
#include <mbedtls/sha256.h>

#define CS_PIN 5
#define BUZZER_PIN 33
#define LED_PIN 32

uint8_t aes_key[32] = {
  0x2b,0x7e,0x15,0x16,0x28,0xae,0xd2,0xa6,
  0xab,0xf7,0x15,0x88,0x09,0xcf,0x4f,0x3c,
  0x2b,0x7e,0x15,0x16,0x28,0xae,0xd2,0xa6,
  0xab,0xf7,0x15,0x88,0x09,0xcf,0x4f,0x3c
};

void setup() {
  Serial.begin(115200);
  while (!Serial);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  if (!SD.begin(CS_PIN)) {
    Serial.println("SD card mount failed");
    while (1) delay(1000);
  }
  Serial.println("SD card mounted");

  // Use tampered hash here to simulate tampering detection
  decryptAndVerify("/encrypted.dat", "/hash_tampered.dat", "/imagedecrypted.jpg");
}

void loop() {}

void decryptAndVerify(const char* encFilePath, const char* hashFilePath, const char* decFilePath) {
  File encFile = SD.open(encFilePath);
  File hashFile = SD.open(hashFilePath);

  if (!encFile || !hashFile) {
    Serial.println("Failed to open encrypted or hash file");
    alertTamper();
    return;
  }

  size_t origSize;
  encFile.read((uint8_t*)&origSize, sizeof(origSize));
  size_t paddedSize = ((origSize + 15) / 16) * 16;

  uint8_t* encBuf = (uint8_t*)malloc(paddedSize);
  uint8_t* decBuf = (uint8_t*)malloc(paddedSize);
  uint8_t storedHash[32];
  uint8_t calcHash[32];

  if (!encBuf || !decBuf) {
    Serial.println("Memory allocation failed");
    alertTamper();
    return;
  }

  size_t bytesRead = encFile.read(encBuf, paddedSize);
  hashFile.read(storedHash, 32);
  encFile.close();
  hashFile.close();

  if (bytesRead != paddedSize) {
    Serial.println("Failed to read full encrypted file");
    alertTamper();
    free(encBuf);
    free(decBuf);
    return;
  }

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_dec(&aes, aes_key, 256);

  for (size_t i = 0; i < paddedSize; i += 16) {
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, encBuf + i, decBuf + i);
  }
  mbedtls_aes_free(&aes);

  mbedtls_sha256_context sha_ctx;
  mbedtls_sha256_init(&sha_ctx);
  mbedtls_sha256_starts(&sha_ctx, 0);
  mbedtls_sha256_update(&sha_ctx, decBuf, origSize);
  mbedtls_sha256_finish(&sha_ctx, calcHash);
  mbedtls_sha256_free(&sha_ctx);

  bool tamperingDetected = false;
  for (int i = 0; i < 32; i++) {
    if (storedHash[i] != calcHash[i]) {
      tamperingDetected = true;
      Serial.printf("Hash mismatch at byte %d: stored 0x%02X != calc 0x%02X\n", i, storedHash[i], calcHash[i]);
    }
  }

  if (tamperingDetected) {
    Serial.println("!!! TAMPERING DETECTED !!!");
    alertTamper();
  } else {
    Serial.println("Decryption and verification successful.");
    alertSuccess();

    File decFile = SD.open(decFilePath, FILE_WRITE);
    if (!decFile) {
      Serial.println("Failed to open decrypted output file");
      free(encBuf);
      free(decBuf);
      return;
    }
    decFile.write(decBuf, origSize);
    decFile.close();
  }

  free(encBuf);
  free(decBuf);
}

void alertTamper() {
  for (int i = 0; i < 10; i++) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}

void alertSuccess() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}

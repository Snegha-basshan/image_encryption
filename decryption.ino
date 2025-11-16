
#include "FS.h"
#include "SD.h"
#include <SPI.h>
#include <mbedtls/aes.h>
#include <mbedtls/sha256.h>

#define CS_PIN 5
#define BUZZER_PIN 25
#define LED_PIN 26

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

  decryptAndVerify("/encrypted.dat", "/hash.dat", "/imagedecrypted.jpg");
}

void loop() {}

void decryptAndVerify(const char *encPath, const char *hashPath, const char *outPath) {
  File encFile = SD.open(encPath);
  File hashFile = SD.open(hashPath);

  if (!encFile || !hashFile) {
    Serial.println("Encrypted or hash file missing!");
    alertTamper();
    return;
  }

  size_t origSize;
  encFile.read((uint8_t *)&origSize, sizeof(origSize));

  size_t paddedSize = ((origSize + 15) / 16) * 16;

  uint8_t *encBuf = (uint8_t *)malloc(paddedSize);
  uint8_t *decBuf = (uint8_t *)malloc(paddedSize);
  uint8_t hashStored[32];
  uint8_t hashCalc[32];

  if (!encBuf || !decBuf) {
    Serial.println("Memory allocation failed");
    alertTamper();
    return;
  }

  size_t readBytes = encFile.read(encBuf, paddedSize);
  hashFile.read(hashStored, 32);

  encFile.close();
  hashFile.close();

  if (readBytes != paddedSize) {
    Serial.println("Encrypted file read error");
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
  mbedtls_sha256_finish(&sha_ctx, hashCalc);
  mbedtls_sha256_free(&sha_ctx);

  bool match = true;
  for (int i = 0; i < 32; i++) {
    if (hashStored[i] != hashCalc[i]) {
      match = false;
      break;
    }
  }

  if (!match) {
    Serial.println("Tampering detected! Hash mismatch.");
    alertTamper();
    free(encBuf);
    free(decBuf);
    return;
  }

  File outFile = SD.open(outPath, FILE_WRITE);
  if (!outFile) {
    Serial.println("Failed to open output file");
    free(encBuf);
    free(decBuf);
    return;
  }
  outFile.write(decBuf, origSize);
  outFile.close();

  Serial.println("Decryption and hash verification successful.");
  alertSuccess();

  free(encBuf);
  free(decBuf);
}

void alertTamper() {
  for (int i=0; i<10; i++) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}

void alertSuccess() {
  for (int i=0; i<3; i++) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}

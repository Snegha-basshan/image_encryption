#include "FS.h"
#include "SD.h"
#include <SPI.h>
#include <mbedtls/aes.h>

#define CS_PIN 5  // SD card chip select pin

// 256-bit key (Please change to your own secure key)
uint8_t aes_key[32] = {
  0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
  0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
  0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
  0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  if (!SD.begin(CS_PIN)) {
    Serial.println("SD Card mount failed");
    while (true) { delay(500); }
  }
  Serial.println("SD Card mounted successfully");

  encryptFile("/project_images/image.jpg", "/encrypted.dat");
}

void loop() {
  // nothing here
}

void encryptFile(const char *inputPath, const char *outputPath) {
  File inputFile = SD.open(inputPath, FILE_READ);
  if (!inputFile) {
    Serial.println("Failed to open input file");
    return;
  }

  size_t fileSize = inputFile.size();
  size_t paddedSize = ((fileSize + 15) / 16) * 16;

  uint8_t *buffer = (uint8_t *)malloc(paddedSize);
  uint8_t *encryptedBuffer = (uint8_t *)malloc(paddedSize);

  if (!buffer || !encryptedBuffer) {
    Serial.println("Failed to allocate memory for file buffer");
    inputFile.close();
    if (buffer) free(buffer);
    if (encryptedBuffer) free(encryptedBuffer);
    return;
  }

  inputFile.read(buffer, fileSize);
  inputFile.close();

  // padding with zeroes for the last block
  for (size_t i = fileSize; i < paddedSize; i++) buffer[i] = 0;

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, aes_key, 256);
  for (size_t i = 0; i < paddedSize; i += 16) {
      mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, buffer + i, encryptedBuffer + i);
  }
  mbedtls_aes_free(&aes);

  File outputFile = SD.open(outputPath, FILE_WRITE);
  if (!outputFile) {
    Serial.println("Failed to create output file");
    free(buffer);
    free(encryptedBuffer);
    return;
  }

  // Store actual file size for decryption
  outputFile.write((uint8_t *)&fileSize, sizeof(fileSize));
  outputFile.write(encryptedBuffer, paddedSize);
  outputFile.close();

  free(buffer);
  free(encryptedBuffer);

  Serial.println("Encryption completed successfully!");
}


#include "FS.h"
#include "SD.h"
#include <SPI.h>
#include <mbedtls/sha256.h>

#define CS_PIN 5  // SD card chip select pin

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  if (!SD.begin(CS_PIN)) {
    Serial.println("SD Card mount failed");
    while (true) { delay(500); }
  }
  Serial.println("SD Card mounted successfully");

  generateHash("/project_images/image.jpg", "/hash.dat");
}

void loop() {
  // nothing here
}

void generateHash(const char *inputPath, const char *outputPath) {
  File inputFile = SD.open(inputPath, FILE_READ);
  if (!inputFile) {
    Serial.println("Failed to open input file");
    return;
  }

  size_t fileSize = inputFile.size();
  uint8_t *buffer = (uint8_t *)malloc(fileSize);
  if (!buffer) {
    Serial.println("Failed to allocate memory for file buffer");
    inputFile.close();
    return;
  }

  inputFile.read(buffer, fileSize);
  inputFile.close();

  uint8_t outputHash[32];
  mbedtls_sha256_context sha_ctx;
  mbedtls_sha256_init(&sha_ctx);
  mbedtls_sha256_starts(&sha_ctx, 0);
  mbedtls_sha256_update(&sha_ctx, buffer, fileSize);
  mbedtls_sha256_finish(&sha_ctx, outputHash);
  mbedtls_sha256_free(&sha_ctx);

  File hashFile = SD.open(outputPath, FILE_WRITE);
  if (!hashFile) {
    Serial.println("Failed to create hash file");
    free(buffer);
    return;
  }

  hashFile.write(outputHash, 32);
  hashFile.close();

  free(buffer);

  Serial.print("SHA-256 hash:");
  for (int i = 0; i < 32; i++) {
    if (outputHash[i] < 16) Serial.print("0");
    Serial.print(outputHash[i], HEX);
  }
  Serial.println("\nHash saved successfully!");
}


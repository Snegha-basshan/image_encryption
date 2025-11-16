#include "FS.h"
#include "SD.h"
#include <SPI.h>

#define CS_PIN 5  // Adjust as per your wiring

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!SD.begin(CS_PIN)) {
    Serial.println("SD card mount failed");
    while (1) delay(1000);
  }
  Serial.println("SD card mounted.");

  checkFileExists("/encrypted.dat");
  checkFileExists("/hash.dat");
  checkFileExists("/imagedecrypted.jpg");
}

void loop() {
  // Nothing here
}

void checkFileExists(const char* filename) {
  if (SD.exists(filename)) {
    Serial.print(filename);
    Serial.println(" found on SD card.");
  } else {
    Serial.print(filename);
    Serial.println(" NOT found on SD card.");
  }
}

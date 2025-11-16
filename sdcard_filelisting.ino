
#include "FS.h"
#include "SD.h"
#include <SPI.h>

#define CS_PIN 5  // Change as per your setup

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Initializing SD card...");

  if (!SD.begin(CS_PIN)) {
    Serial.println("SD card initialization failed!");
    while (true) {
      delay(1000);
    }
  }
  Serial.println("SD card initialized.");

  listFiles("/", 0);
}

void loop() {
  // Nothing to do here
}

void listFiles(const char * dirname, uint8_t levels){
  File root = SD.open(dirname);
  if(!root){
    Serial.println("Failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return;
  }
  
  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      Serial.print("DIR : ");
      Serial.println(file.name());
      if(levels){
        listFiles(file.name(), levels - 1);
      }
    } else {
      Serial.print("FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

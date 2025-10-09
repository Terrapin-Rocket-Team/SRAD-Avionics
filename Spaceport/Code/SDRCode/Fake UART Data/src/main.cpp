#include <Arduino.h>
#include <SD.h>
#define SD_CS_PIN 10 //change to SD card CS pin

File file;
int count = 0;
bool ping = false;
// increments log and telemetry messages
String getMessages(){
  
  file = SD.open("log.txt"); 

  // go to count th line
  for(int i=0;i<count;i++){
    String line = file.readStringUntil('\n');
  }
  
  String logMessage = file.readStringUntil('\n');
  count++;

  return logMessage;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  SD.begin(SD_CS_PIN);
}

void loop() {
  // get ping
  if (!ping && Serial1.available()) {
    String incoming = Serial1.readStringUntil('\n');
    incoming.trim();f

    if (incoming.equalsIgnoreCase("ping")) {
      const char* pongMsg = "pong";
      Serial1.write((const uint8_t*)pongMsg, strlen(pongMsg));
      ping = true;
    }
  }

  // write messages
  String msg = getMessages();
  Serial1.write((const uint8_t*)msg.c_str(), msg.length()); 

  // send at 50 hz
  delay(20);
}
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
    incoming.trim();

    if (incoming.equalsIgnoreCase("ping")) {
      Serial1.write("pong");
      ping = true;
    }
  }

  // write messages
  Serial1.write(getMessages().c_str());

  // send at 50 hz
  delay(20);
}
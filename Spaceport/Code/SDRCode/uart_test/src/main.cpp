#include <Arduino.h>
#include <SD.h>

File file;
int count = 0;
bool ping = false;
// increments log and telemetry messages
String getMessages(){
  return file.readStringUntil('\n');
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial1.begin(115200);
  SD.begin(BUILTIN_SDCARD);
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  file = SD.open("fake_log.txt"); 
}

void loop() {
  // get ping
  if (!ping && Serial1.available()) {
    String incoming = Serial1.readStringUntil('\n');
    Serial.print(incoming);
    incoming.trim();

    if (incoming.equalsIgnoreCase("ping")) {
      Serial.print("Recieved ping");
      Serial1.println("pong");
      ping = true;
    }
  }

  if (ping) {
    // write messages
    String msg = getMessages();
    Serial1.println(msg); 
    Serial.println(msg); 
    Serial.flush();
    Serial1.flush();

    // (const uint8_t*)msg.c_str(), msg.length()

    // send at 50 hz
    delay(20);
  }


}
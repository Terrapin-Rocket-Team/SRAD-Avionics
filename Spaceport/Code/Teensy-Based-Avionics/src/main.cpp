#include <Arduino.h>
#include "AvionicsKF.h"
#include "Sensors/IMU/BMI088andLIS3MDL.h"
#include "Sensors/Baro/DPS368.h"
#include "Sensors/Accel/BMI088Accel.h"
#include "Sensors/Gyro/BMI088Gyro.h"
#include "State/State.h"
#include "Utils/Astra.h"
#include "RetrieveData/SerialHandler.h"
#include "RecordData/Logging/EventLogger.h"
#include "RecordData/Logging/DataLogger.h"
#include "Type_2GT.h"
#include "Sensors/GPS/MAX_M10S.h"

using namespace astra;

const int BUZZER_PIN = 33;
const int PIN_24_ENABLE = 7;
int LED_GPS = 32;  // Teensy pin mapping
int leds[] = {LED_GPS};

Type2GT rad(10, 21, 20, 7, SPI);
MAX_M10S g;
BMI088Accel acc;
BMI088Gyro gyro;
DPS368 baro;
Sensor *sensors[] = {&g, &acc, &gyro, &baro};

AvionicsKF kfilter;
State avionicsState(sensors, sizeof(sensors) / 4, &kfilter);

CircBufferLog buf(5000, true);
USBLog uLog2(Serial, 11200, true);
ILogSink *bufLogs[] = {&buf, &uLog2};
UARTLog uLog(Serial8, 500000, true);
ILogSink *logs[] = {&uLog, &uLog2};

AstraConfig config = AstraConfig()
                         .withBBPin(LED_GPS)
                         .withBuzzerPin(BUZZER_PIN)
                         .withDataLogs(logs, 2)
                         .withState(&avionicsState);

Astra sys(&config);

int startAlt = 0;
uint32_t startTime = 0;
bool pin24Enabled = false;

void radInt(void)
{
  rad.respondToIrq();
//   bb.off(LED_BUILTIN);
}

void setup()
{
  delay(1000);
  Wire.begin();
  SPI.begin();
  pinMode(PIN_24_ENABLE, OUTPUT);
  digitalWrite(PIN_24_ENABLE, LOW);
  
  int r = rad.begin();
  if (r == RADIOLIB_ERR_NONE)
    Serial.println("Radio Init OK");
  else
    Serial.printf("Radio Init FAIL %d\n", r);
  
  rad.onIrq(radInt);
  rad.recieve();
  
  Serial.println("Starting up");
  EventLogger::configure(bufLogs, 2);
  sys.init();
  bb.init(leds, 2, true);
  bb.aonoff(LED_GPS, BBPattern(200, 1), true);
  
  for (int i = 0; i < 25; i++)
  {
    baro.update();
    delay(20);
  }
  
  startAlt = baro.getASLAltFt();
  LOGI("Start Altitude: %.2f ft", startAlt);
  
  // Record start time for pin 24 timer
  startTime = millis();
}

bool handshake = false;
bool hasFix = false;
uint32_t last_ms = 0;

void loop()
{
  sys.update();
  
  // Enable pin 24 after 30 seconds
  if (!pin24Enabled && (millis() - startTime >= 30000))
  {
    digitalWrite(PIN_24_ENABLE, HIGH);
    pin24Enabled = true;
    LOGI("Pin 24 enabled at 30 seconds");
    bb.clearQueue(LED_GPS);
    bb.on(LED_GPS);
  }

  if (!handshake)
    if (Serial8.available())
    {
      String s = Serial8.readStringUntil('\n');
      Serial.println("Received from Pi: " + s);
      s.trim();
      if (s == "PING")
      {
        Serial.println("Received PING from Pi");
        Serial8.println("PONG");
        handshake = true;
        EventLogger::configure(logs, 1);
        buf.transfer(uLog);
        LOGI("Pi Handshake Complete.");
      }
    }
  
  const uint32_t now = millis();
  if ((uint32_t)(now - last_ms) >= 500)
  {
    last_ms = now;
    g.update();
    
    // Create telemetry string
    char str[200];
    snprintf(str, 200, "TELEM2/%.3f,%.2f,%.2f,%.2f,%.7f,%.7f\n", 
             now / 1000.0f, 
             baro.getASLAltFt() - startAlt, 
            10.00,//  avionicsState.getVelocity().magnitude(), 
             acc.getAccel().magnitude(), 
             g.getPos().x(), 
             g.getPos().y());
    
    // Send over USB Serial
    
    // Send over Serial8
    
    // Send over radio
    rad.transmit(str);
    Serial.print(str);
    
    // LED state management
    const bool fix = g.getHasFix();
    if (!hasFix && fix)
    {
      bb.on(LED_GPS);
      hasFix = true;
    }
    else if (hasFix && !fix)
    {
      bb.aonoff(LED_GPS, BBPattern(200, 1), true);
      hasFix = false;
    }
  }
}
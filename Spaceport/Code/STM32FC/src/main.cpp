#include <Arduino.h>
#include "AvionicsKF.h"
#include "SAM_M8Q.h"
#include "Sensors/IMU/BMI088andLIS3MDL.h"
#include "Sensors/Baro/DPS368.h"
#include "Sensors/Accel/BMI088Accel.h"
#include "H3LIS331DL.h"
#include "Sensors/Gyro/BMI088Gyro.h"
#include "State/State.h"
#include "Utils/Astra.h"
#include "RetrieveData/SerialHandler.h"
#include "RecordData/Logging/EventLogger.h"
#include "RecordData/Logging/DataLogger.h"
#include "Type_2GT.h"

using namespace astra;
const int BUZZER_PIN = 33;
int LED_GPS = PB12;
int LED_SENS = PB11;

int leds[] = {LED_GPS, LED_SENS};
Type2GT rad(PA15, PA2, PA6, PA7, SPI);
SAM_M8Q g("SAM-M8Q");
BMI088Accel acc;
BMI088Gyro gyro;
DPS368 baro;
H3LIS331DL acc2;
Sensor *sensors[] = {&g, &acc, &acc2, &gyro, &baro};
AvionicsKF kfilter;
State avionicsState(sensors, sizeof(sensors) / 4, &kfilter);

CircBufferLog buf(5000, true);
ILogSink *bufLogs[] = {&buf};
UARTLog uLog(Serial1, 115200, true);
ILogSink *logs[] = {&uLog};

AstraConfig config = AstraConfig()
                         .withBBPin(LED_BUILTIN)
                         .withBuzzerPin(BUZZER_PIN)
                         .withDataLogs(logs, 1)
                         .withState(&avionicsState);

Astra sys(&config);

int startAlt = 0;
void radInt(void)
{
  rad.respondToIrq();
  bb.off(LED_SENS);
}

void setup()
{
  Wire.setSDA(PB9);
  Wire.setSCL(PB8);
  Wire.begin();

  Serial.setTx(PB6_ALT2);
  Serial.setRx(PB7_ALT1);
  Serial.begin(115200);

  SPI.setMISO(PB4);
  SPI.setSCLK(PB3);
  SPI.setMOSI(PD7);
  SPI.begin();

  int r = rad.begin();
  if (r == RADIOLIB_ERR_NONE)
    Serial.println("Radio Init OK");
  else
    Serial.printf("Radio Init FAIL %d\n", r);

  rad.onIrq(radInt);
  // Optional: prime RX so IRQ path is exercised even before first TX
  rad.recieve();

  Serial.println("Starting up");

  EventLogger::configure(bufLogs, 1);

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
}
bool handshake = false;
bool hasFix = false;
uint32_t last_ms = 0;
void loop()
{
  sys.update();
  if (!handshake)
    if (Serial1.available())
    {
      String s = Serial1.readStringUntil('\n');
      s.trim();
      if (s == "PING")
      {
        Serial1.println("PONG");
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

    // Emit a single, parseable line with newline
    char str[200];
    // send alt, speed, acc, and time.
    snprintf(str, 200, "TELEM2/%.3f,%.2f,%.2f,%.2f,%.7f, %.7f\n", now / 1000.0f, baro.getASLAltFt() - startAlt, avionicsState.getVelocity().magnitude(), acc.getAccel().magnitude(), g.getPos().x(), g.getPos().y());
    Serial.print(str);

    rad.transmit(str);
    // LED state
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

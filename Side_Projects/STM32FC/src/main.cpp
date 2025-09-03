#include <Arduino.h>
#include <MMFS.h>
#include "RCRState.h"
#include "Type_2GT.h"

using namespace mmfs;

DPS310 b;
SAM_M8Q g("SAM-M8Q");
BMI088Gyro y;
BMI088Accel a;
H3LIS331DL c;
MMC5633 m;

Type2GT rad(PA15, PA2, PA6, PA7, SPI);

Sensor *s[] = {&b, &g, &y, &a, &c, &m};

RCRState st(s, sizeof(s) / 4, nullptr);

MMFSConfig co = MMFSConfig()
                    .withBBAsync(true)
                    .withBBPin(PB11)
                    .withBBPin(PB12)
                    .withState(&st)
                    .withUsingSensorBiasCorrection(false)
                    .withLoggingInterval(1000);

MMFSSystem sys(&co);

void radInt(void)
{
  rad.respondToIrq();
}

void setup()
{
  Wire.setSDA(PB9);
  Wire.setSCL(PB8);
  Wire.begin(); // now I2C uses PB9/PB8
  Serial.setTx(PB6_ALT2);
  Serial.setRx(PB7_ALT1);
  Serial.begin(115200);
  // Serial.println("Init...");
  // SPI.setMISO(PB4);
  // SPI.setSCLK(PB3);
  // SPI.setMOSI(PD7);
  // SPI.setSSEL(PA15);
  // SPI.begin();
  // if (rad.begin() == RADIOLIB_ERR_NONE)
  //   Serial.print("Radio Init!");
  // else
  //   Serial.println("No Radio!");
  // rad.onIrq(radInt);

  sys.init();
}

void loop()
{
  sys.update();
  // Serial.print("ASD");
  // delay(50);
}

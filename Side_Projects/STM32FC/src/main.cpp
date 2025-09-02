#include <Arduino.h>
#include <MMFS.h>
#include "RCRState.h"

using namespace mmfs;

DPS310 b;
SAM_M8Q g;
BMI088Gyro y;
BMI088Accel a;
H3LIS331DL c;
MMC5633 m;



Sensor *s[] = {&b, &g, &y, &a, &c, &m};

RCRState st(s, sizeof(s) / 4, nullptr);

MMFSConfig co = MMFSConfig()
                    .withBBAsync(true)
                    .withBBPin(PB11)
                    .withBBPin(PB12)
                    .withState(&st)
                    .withUsingSensorBiasCorrection(false)
                    .withUpdateInterval(1000);

MMFSSystem sys(&co);

void setup()
{
  Wire.setSDA(PB9);
  Wire.setSCL(PB8);
  Wire.begin(); // now I2C uses PB9/PB8
  Serial.setTx(PB6_ALT2);
  Serial.setRx(PB7_ALT1);
  Serial.begin(115200);
  sys.init();
}

void loop()
{
  sys.update();
  // Serial.print("ASD");
  // delay(50);
}

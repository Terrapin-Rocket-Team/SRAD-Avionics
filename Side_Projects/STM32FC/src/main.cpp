#include <Arduino.h>
#include <MMFS.h>
#include "RCRState.h"
#include "Type_2GT.h"
#include "stm32h7xx.h"
using namespace mmfs;

DPS310 b;
SAM_M8Q g;
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
                    .withUpdateInterval(1000);

MMFSSystem sys(&co);

void radInt(void)
{
  rad.respondToIrq();
}

static void free_PB3_for_SPI1_SCK()
{
  // Disable trace pins (SWO etc.). Some cores lack DBGMCU_CR_TRACE_IOEN, so clear bit 5 directly.
#ifdef DBGMCU
  DBGMCU->CR &= ~(1UL << 5); // TRACE_IOEN = 0  (bit 5)
#endif
  // Also disable core trace blocks so the debugger can’t re-enable SWO mid-session.
  CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;

  // Now hard-mux PB3 to AF5 (SPI1_SCK)
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef gi{};
  gi.Pin = GPIO_PIN_3;
  gi.Mode = GPIO_MODE_AF_PP;
  gi.Pull = GPIO_NOPULL; // or PULLDOWN for MODE0 idle-low
  gi.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gi.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOB, &gi);
}

void setup()
{

  free_PB3_for_SPI1_SCK();
  Wire.setSDA(PB9);
  Wire.setSCL(PB8);
  Wire.begin(); // now I2C uses PB9/PB8
  Serial.setTx(PB6_ALT2);
  Serial.setRx(PB7_ALT1);
  Serial.begin(115200);
  Serial.println("Init...");
  SPI.setMISO(PB4);
  SPI.setSCLK(PB3);
  SPI.setMOSI(PD7);
  SPI.setSSEL(PA15);
  SPI.begin();
  Serial.printf("SPI1 CFG1=%08lX CFG2=%08lX CR1=%08lX CR2=%08lX SR=%08lX\n",
                SPI1->CFG1, SPI1->CFG2, SPI1->CR1, SPI1->CR2, SPI1->SR);

  Serial.println("SPI Began...");
  if (rad.begin() == RADIOLIB_ERR_NONE)
    Serial.print("Radio Init!");
  else
    Serial.println("No Radio!");
  rad.onIrq(radInt);

  sys.init();
}

void loop()
{
  sys.update();
  // Serial.print("ASD");
  // delay(50);
}

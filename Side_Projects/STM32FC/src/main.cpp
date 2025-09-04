#include <Arduino.h>
#include "RCRState.h"
#include <Sensors/GPS/SAM_M8Q.h>
#include <BlinkBuzz/BlinkBuzz.h>
#include "Type_2GT.h"

using namespace mmfs;

BlinkBuzz bb;
SAM_M8Q g("SAM-M8Q");
// cs, irq, rst, bsy
Type2GT rad(PA15, PA2, PA6, PA7, SPI);
int LED_GPS = PB12;
int LED_SENS = PB11;

int leds[] = {LED_GPS, LED_SENS};

void radInt(void)
{
  rad.respondToIrq();
  bb.off(LED_SENS);
}

void setup()
{
  Serial.setTx(PB6_ALT2);
  Serial.setRx(PB7_ALT1);
  Serial.begin(115200);
  Serial.println("Init...");

  Wire.setSDA(PB9);
  Wire.setSCL(PB8);
  Wire.begin();

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

  if (g.begin())
    Serial.println("GPS Init!");
  else
    Serial.println("No GPS!");

  bb.init(leds, 2, true);
  bb.aonoff(LED_GPS, BBPattern(200, 1), true);
}

double last = 0;
bool hasFix = false;

void pumpBtToLoRa()
{
  static char buf[256]; // slightly bigger – you’ll want to see truncation if it happens
  while (Serial.available())
  {
    bb.on(LED_SENS);
    size_t n = Serial.readBytesUntil('\n', buf, sizeof(buf) - 1);
    if (n == 0)
      break;
    buf[n] = '\0';

    if (strncmp(buf, "LoRa", 4) == 0)
    {
      int rc = rad.transmit(buf);
      Serial.printf("DBG: transmit rc=%d (len=%u)\n", rc, (unsigned)strlen(buf + 3));
      // optional: flash LED_SENS briefly so you can see TX attempts
      bb.off(LED_SENS);
    }
    else
    {
      Serial.printf("DBG: ignoring line: %s\n", buf);
      bb.off(LED_SENS);
    }
  }
}

uint32_t last_ms = 0;

void loop()
{
  bb.update();

  const uint32_t now = millis();
  if ((uint32_t)(now - last_ms) >= 500)
  {
    last_ms = now;
    g.update();

    // Emit a single, parseable line with newline
    Serial.printf("GPS,%.7f,%.7f,%.2f,%u\n",
                  g.getPos().x(), g.getPos().y(), g.getPos().z(), g.getFixQual());

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

  // Non-blocking RX from BT
  pumpBtToLoRa();
}

#include <Arduino.h>

#ifdef STM32

#define STATUS_LED PB12
#define RADIO_NRST PA6
#define RADIO_BUSY PA7
#define RADIO_NCS PA15
#define RADIO_IO8 PA3
// irq pin
#define RADIO_IO9 PA2

#define RADIO_MOSI PD7
#define RADIO_MISO PB4
#define RADIO_SCK PB3

#include "Type_2GT.h"
// create the SPI class to input into the radio
SPIClass Radio_SPI(RADIO_MOSI, RADIO_MISO, RADIO_SCK);
Type2GT radio(RADIO_NCS, RADIO_IO9, RADIO_NRST, RADIO_BUSY, Radio_SPI);

void radInt(void)
{
  radio.respondToIrq();
  digitalWrite(STATUS_LED, LOW);
}

void setup()
{

  // Serial1 (USART) pins
  Serial.setRx(PB7_ALT1); // alt pin defs
  Serial.setTx(PB6_ALT2);
  Serial.begin(115200);

  pinMode(STATUS_LED, OUTPUT);
  int radio_init_statuscode = radio.begin();
  if (radio_init_statuscode != RADIOLIB_ERR_NONE)
  {
    Serial.printf("RAD/Error: Radio not initialized, Error code %d\n", radio_init_statuscode);
  }
  else
  {
    Serial.println("RAD/Info: Radio Initialized");
    digitalWrite(STATUS_LED, HIGH);
    delay(100);
  }
  radio.onIrq(radInt);
  radio.recieve();
}

void loop()
{
  // 2 kb buffer
  char buf[2048];
  if (Serial.available())
  {
    digitalWrite(STATUS_LED, HIGH);
    int i = Serial.readBytesUntil('\n', buf, 2048);  // Fixed: use actual buffer size
    buf[i] = '\0';
    if (!strncmp(buf, "RAD/PING", 8))
    {
      Serial.println("RAD/PONG");
    }
    else
    {
      radio.transmit(buf);
    }
  }
  digitalWrite(STATUS_LED, LOW);
}
#endif
/*
  Dont worry about this code, it is just a USB to serial bridge

*/
#ifdef TEENSY

unsigned long previousMillis = 0;
unsigned long interval = 100;
void setup()
{
  Serial.begin(115200);
  Serial1.begin(115200);
}

void loop()
{

  if (Serial1.available())
  {
    Serial.write((char)Serial1.read());
  }

    if (Serial.available())
  {
    Serial1.write((char)Serial.read());
  }
}

#endif
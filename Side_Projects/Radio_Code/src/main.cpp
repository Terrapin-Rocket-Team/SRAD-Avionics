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
  Serial.setTimeout(5000);
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
  // TX-only bridge path.
}

void loop()
{
  static char buf[2048];
  static size_t buf_len = 0;

  while (Serial.available())
  {
    const int in = Serial.read();
    if (in < 0)
    {
      break;
    }

    const char c = (char)in;
    if (c == '\r' || c == '\0')
    {
      continue;
    }

    if (c != '\n')
    {
      if (buf_len < (sizeof(buf) - 1))
      {
        buf[buf_len++] = c;
      }
      else
      {
        // Overflow guard: drop oversized line and wait for the next newline.
        buf_len = 0;
      }
      continue;
    }

    // Newline received: process one complete line.
    if (buf_len == 0)
    {
      continue;
    }

    digitalWrite(STATUS_LED, HIGH);
    buf[buf_len] = '\0';

    if (!strncmp(buf, "RAD/PING", 8))
    {
      Serial.println("RAD/PONG");
    }
    else
    {
      int tx_status = RADIOLIB_ERR_NONE;
      bool sent = false;
      for (uint8_t attempt = 0; attempt < 4 && !sent; ++attempt)
      {
        tx_status = radio.transmit(buf);
        if (tx_status == RADIOLIB_ERR_NONE)
        {
          sent = true;
        }
        else
        {
          Serial.printf("RAD/Warn: transmit retry attempt=%u err=%d\n",
                        (unsigned)(attempt + 1), tx_status);
          delay(15);
        }
      }
      if (!sent)
      {
        Serial.printf("RAD/Error: transmit failed, Error code %d\n", tx_status);
      }
    }

    buf_len = 0;
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

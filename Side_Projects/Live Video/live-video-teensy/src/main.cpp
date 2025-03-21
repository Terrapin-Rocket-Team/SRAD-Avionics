#include <Arduino.h>

#include "rs.h"

#include "Si4463.h"
#include "SDFatBoilerplate.h"

// radio config header
#include "422Mc110_2GFSK_500000U.h"

#define MSG_SIZE MSG_CHUNK_SIZE * 32 // 8160
#define MSG_THRESH 4000
#define MSG_CHUNK_SIZE 255
#define MSG_CHUNK_DATA_SIZE MSG_CHUNK_SIZE - NPAR // 251

bool sdInit(SdFs &sd, FsFile &f, bool read);
void sdWrite(FsFile &f, const uint8_t *data, int length);
void sdClose(FsFile &f);

// Serial communication with Pi
uint8_t buf[MSG_SIZE * 3];
uint8_t codeword[255];

// radio variables
bool hasTransmission = false;
bool firstTX = true;

int top = 0;
int toSend = 0;
int sent = 0;
int bytesThisMessage = 0;

uint32_t txTimeout = millis();

// testing
uint32_t debugTimer = millis();
// end testing

RS rs;

// radio
APRSConfig aprscfg = {"KC3UTM", "ALL", "WIDE1-1", TextMessage, '\\', 'M'};

Si4463HardwareConfig hwcfg = {
    MOD_2GFSK,       // modulation
    DR_500k,         // data rate
    (uint32_t)433e6, // frequency (Hz)
    127,             // tx power (127 = ~20dBm)
    48,              // preamble length
    16,              // required received valid preamble
};

Si4463PinConfig pincfg = {
    &SPI, // spi bus to use
    10,   // cs
    7,    // sdn
    24,   // irq
    26,   // gpio0
    25,   // gpio1
    8,    // random pin - gpio2 is not connected
    9,    // random pin - gpio3 is not connected
};

Si4463 radio(hwcfg, pincfg);
SdFs s;
FsFile out;

void setup()
{
  Serial.begin(500000);
  Serial5.begin(500000); // 1M baud

  if (CrashReport)
    Serial.println(CrashReport);

  // if (!radio.begin(CONFIG_422Mc110_2GFSK_500000U, sizeof(CONFIG_422Mc110_2GFSK_500000U)))
  // {
  //   Serial.println("Transmitter failed to begin");
  //   Serial.flush();
  //   while (1)
  //     ;
  // }

  if (MSG_THRESH > MSG_SIZE)
  {
    Serial.println("Sending threshold greater than message size!");
    Serial.flush();
    while (1)
      ;
  }

  // sdInit(s, out, false);
  // sdWrite(s, out, (const uint8_t *)"hello", sizeof("hello"));

  // Serial.println("Setup complete");
}

void loop()
{

  // reading from Raspi
  while (Serial5.available() > 0 && top + 5 < MSG_SIZE * 3)
  {
    txTimeout = millis();
    buf[top] = Serial5.read();
    top++;

    if (bytesThisMessage + toSend < MSG_SIZE && hasTransmission)
    {
      toSend++;
    }

    // add RS once we have MSG_CHUNK_SIZE bytes
    if (top != 0 && top % MSG_CHUNK_DATA_SIZE == 0)
    {
      // Serial.println("RS");
      rs.encode_data(buf + (top - MSG_CHUNK_DATA_SIZE), MSG_CHUNK_DATA_SIZE, codeword);
      // for (int i = 0; i < MSG_CHUNK_SIZE; i++)
      // {
      //   Serial.print((buf + (top - MSG_CHUNK_SIZE))[i], HEX);
      //   Serial.print(" ");
      // }
      // Serial.println();
      // for (int i = 0; i < sizeof(codeword); i++)
      // {
      //   Serial.print(codeword[i], HEX);
      //   Serial.print(" ");
      // }
      // Serial.println();
      memcpy(buf + (top - MSG_CHUNK_DATA_SIZE), codeword, sizeof(codeword));
      top += 4;
    }
  }

  if (top >= MSG_SIZE * 3)
  {
    Serial.print("Buffer overrun!\ttop ");
    Serial.println(top);
  }
  if (toSend >= MSG_SIZE)
  {
    Serial.print("Sending too many bytes!\ttoSend ");
    Serial.println(toSend);
  }

  // Refill fifo here
  if (hasTransmission && toSend > 0)
  {
    // wrong
    // radio.writeTXBuf(buf + bytesThisMessage, toSend);
    // sdWrite(out, buf + bytesThisMessage, toSend);
    // correct
    Serial.write(buf + bytesThisMessage, toSend);
    sent = toSend;
    bytesThisMessage += toSend;
    toSend = 0;
  }

  // Start a new transmission
  if ((top >= MSG_THRESH || (millis() - txTimeout > 100 && top > 0 && !firstTX)) && !hasTransmission)
  {
    firstTX = false;
    hasTransmission = true;
    toSend = (top > MSG_SIZE) ? MSG_SIZE : top;

    Serial.write(buf + bytesThisMessage, toSend);
    // radio.startTX(buf + bytesThisMessage, toSend, MSG_SIZE);
    // sdWrite(out, buf + bytesThisMessage, toSend);

    // set all status vars
    sent = toSend;
    bytesThisMessage += toSend;
    toSend = 0;
  }

  // if (millis() - txTimeout > 100 && top > 0)
  // {
  //   top = 0;
  //   sent = 0;
  // }

  if ((bytesThisMessage == MSG_SIZE || (millis() - txTimeout > 100 && toSend == 0 && hasTransmission)))
  {
    // remove sent bytes
    top -= bytesThisMessage;
    memcpy(buf, buf + bytesThisMessage, top);
    bytesThisMessage = 0;
    if (millis() - txTimeout > 100 && toSend == 0 && hasTransmission)
    {
      // sdClose(out);
      // Serial.println("finished");
    }
    hasTransmission = false;
  }

  // if (millis() - debugTimer > 100)
  // {
  //   debugTimer = millis();
  //   Serial.print("\r                                                                                                   ");
  //   Serial.print("\rBuffer state: ");
  //   Serial.print("\ttop ");
  //   Serial.print(top);
  //   Serial.print("\tsent ");
  //   Serial.print(sent);
  //   Serial.print("\ttoSend ");
  //   Serial.print(toSend);
  //   Serial.print("\tbytesThisMessage ");
  //   Serial.print(bytesThisMessage);
  // }

  radio.update();

  if (radio.state == STATE_TX && radio.availLen > 0 && radio.availLen == radio.xfrd && radio.xfrd < MSG_SIZE && radio.gpio0())
  {
    Serial.print("\nRan out of bits!\tbytesThisMessage ");
    Serial.print(bytesThisMessage);
    Serial.print("\ttop ");
    Serial.print(top);
    Serial.print("\tavailLen ");
    Serial.print(radio.availLen);
    Serial.print("\txfrd ");
    Serial.print(radio.xfrd);
    Serial.print("\tstate ");
    Serial.println(radio.state);
    while (1)
      ;
  }
}

bool sdInit(SdFs &sd, FsFile &f, bool read)
{
  if (sd.begin(SD_CONFIG) || sd.restart())
  {
    if (read)
    {
      f = sd.open("out.av1", FILE_READ);
    }
    else
    {
      if (sd.exists("out.av1"))
        sd.remove("out.av1");
      f = sd.open("out.av1", FILE_WRITE);
    }
    return true;
  }
  return false;
}

void sdWrite(FsFile &f, const uint8_t *data, int length)
{
  if (length > 0)
  {
    f.write(data, length);
  }
}

void sdClose(FsFile &f)
{
  f.close();
}
#include <Arduino.h>
#include "Wire.h"

#include "rs.h"

#include "Si4463.h"
#include "MockRadio.h"
#include "RadioMessage.h"

// radio config header
#include "422Mc110_2GFSK_500000U.h"

// pin definitions
#define BUZZER 33
#define LED 32

// data flow constants
#define MSG_CHUNK_SIZE 255
#define MSG_SIZE (MSG_CHUNK_SIZE * 32)              // 8160 (should be * 32)
#define MSG_CHUNK_DATA_SIZE (MSG_CHUNK_SIZE - NPAR) // 245
#define MSG_THRESH (MSG_SIZE)

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
uint32_t debugTimer = micros();
int debugCounter = 0;
uint32_t debugLoopTime = 0;
GSData vHeader(VideoData::type, 3, 1);
uint8_t vHeaderBuf[GSData::headerLen] = {};
// end testing

// Reed solomon
RS rs;
bool disableRS = true;

// radio config
APRSConfig aprscfg = {"KC3UTM", "ALL", "WIDE1-1", TextMessage, '\\', 'M'};

// Si4463HardwareConfig hwcfg = {
//     MOD_2GFSK,        // modulation
//     DR_500k,          // data rate
//     (uint32_t)433e6,  // frequency (Hz)
//     POWER_COTS_30dBm, // tx power
//     48,               // preamble length
//     16,               // required received valid preamble
// };

// Si4463PinConfig pincfg = {
//     &SPI, // spi bus to use
//     10,   // cs
//     2,    // sdn
//     7,    // irq
//     6,    // gpio0
//     5,    // gpio1
//     4,    // gpio2
//     3,    // gpio3
// };

// Si4463 radio(hwcfg, pincfg);

MockHardwareConfig hwcfg = {
    500000};

MockPinConfig pincfg = {
    &Serial6};

MockRadio radio(hwcfg, pincfg);

void beep(int time)
{
  digitalWrite(BUZZER, HIGH);
  delay(time);
  digitalWrite(BUZZER, LOW);
}

void blink(int time)
{
  digitalWrite(LED, HIGH);
  delay(time);
  digitalWrite(LED, LOW);
}

void pattern(void (*f)(int), int time, int loops)
{
  for (int i = 0; i < loops; i++)
  {
    f(time);
  }
}

char GSMHeader[GSData::gsmHeaderSize] = {};

void setup()
{
  Serial.begin(250000);

  // serial
  Serial1.begin(500000);
  // i2c
  // Wire.begin(0x01);

  // setup buzzer and led
  pinMode(BUZZER, OUTPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(BUZZER, LOW);
  digitalWrite(LED, LOW);

  if (CrashReport)
    Serial.println(CrashReport);

  // if (!radio.begin(CONFIG_422Mc110_2GFSK_500000U, sizeof(CONFIG_422Mc110_2GFSK_500000U)))
  if (!radio.begin())
  {
    Serial.println("Transmitter failed to begin");
    Serial.flush();
    while (1)
      ;
  }

  if (MSG_THRESH > MSG_SIZE)
  {
    Serial.println("Sending threshold greater than message size!");
    Serial.flush();
    while (1)
      ;
  }

  // using buf because just need a valid arr
  // only need to extract the header
  // vHeader.fill(buf, MSG_SIZE);
  // vHeader.encode(buf, MSG_SIZE);
  // this header should be valid for every packet
  // memcpy(vHeaderBuf, buf, GSData::headerLen);
  // reset buf for good measure
  // memset(buf, 0, MSG_SIZE);

  // write GSM header
  // GSData::encodeGSMHeader(GSMHeader, GSData::gsmHeaderSize, 400000);

  Serial.print("Reed solomon is: ");
  Serial.println(disableRS ? "DISABLED" : "ENABLED");
  Serial.print("Message size is: ");
  Serial.print(MSG_SIZE);
  Serial.println(" bytes");
  Serial.print("Threshold is: ");
  Serial.print(MSG_THRESH);
  Serial.println(" bytes");
  Serial.println("Setup complete");
  pattern(beep, 100, 3);
}

void loop()
{
  debugTimer = micros();
  // reading from Raspi
  if (Serial1.available() > 0 && top + 5 < MSG_SIZE * 3)
  // if (Wire.available() > 0 && top + 5 < MSG_SIZE * 3)
  {
    txTimeout = millis();
    buf[top] = Serial1.read();
    // buf[top] = Wire.read();
    top++;

    if (bytesThisMessage + toSend < MSG_SIZE && hasTransmission)
    {
      toSend++;
    }

    // add RS once we have MSG_CHUNK_SIZE bytes
    if (top != 0 && ((top - (MSG_CHUNK_SIZE * (top / MSG_CHUNK_SIZE))) % MSG_CHUNK_DATA_SIZE) == 0 && !disableRS)
    {
      // (top - (MSG_CHUNK_SIZE * (top / MSG_CHUNK_SIZE))) the number of bytes not part of an already coded message
      // Serial.println("RS");
      // Serial.print("top ");
      // Serial.print(top);
      // Serial.print("\t");
      // Serial.print(MSG_CHUNK_DATA_SIZE);
      // Serial.print("\ttoSend ");
      // Serial.print(toSend);
      // Serial.print("\thasTX ");
      // Serial.println(hasTransmission);
      rs.encode_data(buf + (top - MSG_CHUNK_DATA_SIZE), MSG_CHUNK_DATA_SIZE, codeword);
      // for (int i = 0; i < MSG_CHUNK_DATA_SIZE; i++)
      // {
      //   Serial.print((buf + (top - MSG_CHUNK_DATA_SIZE))[i], HEX);
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
      if (bytesThisMessage + toSend + 4 <= MSG_SIZE && hasTransmission)
      {
        toSend += 4;
      }
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
    radio.writeTXBuf(buf + bytesThisMessage, toSend);
    // Serial.write(buf + bytesThisMessage, toSend);
    sent = toSend;
    bytesThisMessage += toSend;
    toSend = 0;
  }

  // Start a new transmission
  if ((top >= MSG_THRESH || (millis() - txTimeout > 100 && top > 0 && !firstTX)) && !hasTransmission)
  {
    // turn on led
    digitalWrite(LED, HIGH);
    // TODO: remove temp
    // if (firstTX)
    //   Serial.write(GSMHeader, GSData::gsmHeaderSize);

    firstTX = false;
    hasTransmission = true;
    toSend = (top > MSG_SIZE) ? MSG_SIZE : top;
    // TEMP: write GSData header for testing with ground station
    // Serial.write(vHeaderBuf, GSData::headerLen);
    // write data
    // Serial.write(buf + bytesThisMessage, toSend);
    radio.startTX(buf + bytesThisMessage, toSend, MSG_SIZE);

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

  if ((bytesThisMessage == MSG_SIZE || (millis() - txTimeout > 100 && toSend == 0)) && hasTransmission && radio.state == STATE_IDLE)
  {
    // remove sent bytes
    top -= bytesThisMessage;
    memcpy(buf, buf + bytesThisMessage, top);
    bytesThisMessage = 0;
    if (millis() - txTimeout > 100 && toSend == 0 && hasTransmission)
    {
      // sdClose(out);
    }
    hasTransmission = false;
    Serial.println();
    Serial.print("top ");
    Serial.print(top);
    Serial.println(" finished");
    // turn off led
    digitalWrite(LED, LOW);
  }

  radio.update();

  if (millis() - debugTimer > 100)
  {
    debugTimer = millis();
    Serial.print("\r                                                                                                   ");
    Serial.print("\rBuffer state: ");
    Serial.print("\ttop ");
    Serial.print(top);
    Serial.print("\tsent ");
    Serial.print(sent);
    Serial.print("\ttoSend ");
    Serial.print(toSend);
    Serial.print("\tbytesThisMessage ");
    Serial.print(bytesThisMessage);
    Serial.print("\tavailLen ");
    Serial.print(radio.availLen);
    Serial.print("\txfrd ");
    Serial.print(radio.xfrd);
    Serial.print("\tTX_MODE ");
    Serial.print(radio.gpio3());
    Serial.print("\tTX_FIFO_EMPTY ");
    Serial.print(radio.gpio0());
  }

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
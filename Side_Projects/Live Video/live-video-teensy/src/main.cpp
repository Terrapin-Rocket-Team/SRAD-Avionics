#include <Arduino.h>

#include "RFM69HCW.h"

#define MSG_SIZE 25000
#define MSG_THRESH 25000
#define TX_ADDR 0x02
#define RX_ADDR 0x01
#define SYNC1 0x00
#define SYNC2 0xff

void radioIdle();
void radioTx();

// Serial communication with Pi
char buf[MSG_SIZE * 3];

// radio variables
bool hasTransmission = false;
bool modeReady = true;

int top = 0;
int toSend = 0;
int sent = 0;
int bytesThisMessage = 0;

uint32_t txTimeout = millis();

// testing
uint32_t debugTimer = millis();
// end testing

// radio
APRSConfig config = {"KC3UTM", "APRS", "WIDE1-1", '[', '/'};
RadioSettings settings = {433.0, true, true, &hardware_spi, 10, 15, 14, 8, 4, 9};
RFM69HCW transmit = {&settings, &config};

void setup()
{
  Serial.begin(9600);
  Serial1.begin(460800); // 460800 baud

  if (!transmit.begin())
    Serial.println("Transmitter failed to begin");

  if (MSG_THRESH > MSG_SIZE)
  {
    Serial.println("Sending threshold greater than message size!");
    Serial.flush();
    while (1)
      ;
  }

  // Serial.println("RFM69t began");
}

void loop()
{

  // reading from Raspi
  while (top + 1 < MSG_SIZE * 3)
  {
    txTimeout = millis();
    buf[top] = '1';
    top++;

    if (bytesThisMessage + toSend < MSG_SIZE && hasTransmission)
    {
      toSend++;
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
  if (hasTransmission && toSend > 0 && !transmit.FifoFull())
  {
    // Start spi transaction
    settings.spi->beginTransaction();
    // Select radio
    digitalWrite(settings.cs, LOW);

    settings.spi->transfer(RH_RF69_REG_00_FIFO | RH_RF69_SPI_WRITE_MASK);

    // Send the next section of the payload
    sent = 0;
    while (toSend > 0 && !transmit.FifoFull())
    {
      settings.spi->transfer(buf[sent]);
      sent++;
      bytesThisMessage++;
      toSend--;
    }
    memcpy(buf, buf + sent, top - sent);
    top -= sent;

    // Deselect radio
    digitalWrite(settings.cs, HIGH);
    // End spi transaction
    settings.spi->endTransaction();
  }
  else if (hasTransmission && toSend <= 0 && bytesThisMessage < MSG_SIZE)
  {
    Serial.print("Ran out of bits!\tbytesThisMessage ");
    Serial.print(bytesThisMessage);
    Serial.print("\ttop ");
    Serial.println(top);
  }

  // Start a new transmission
  if ((top >= MSG_THRESH || (millis() - txTimeout > 100 && top > 0)) && !hasTransmission && modeReady)
  {
    hasTransmission = true;
    toSend = (top > MSG_SIZE) ? MSG_SIZE : top;

    // Start spi transaction
    settings.spi->beginTransaction();
    // Select radio
    digitalWrite(settings.cs, LOW);

    // Select the fifo for writing
    settings.spi->transfer(RH_RF69_REG_00_FIFO | RH_RF69_SPI_WRITE_MASK);
    // Send message length
    settings.spi->transfer(SYNC1);
    // Send address for receving radio
    settings.spi->transfer(SYNC2);

    // Send the payload
    sent = 0;
    while (toSend > 0 && !transmit.FifoFull())
    {
      settings.spi->transfer(buf[sent]);
      sent++;
      bytesThisMessage++;
      toSend--;
    }
    memcpy(buf, buf + sent, top - sent);
    top -= sent;

    // Deselect radio
    digitalWrite(settings.cs, HIGH);
    // End spi transaction
    settings.spi->endTransaction();

    radioTx();
  }

  if (millis() - txTimeout > 100 && top > 0)
  {
    top = 0;
    sent = 0;
  }

  if ((bytesThisMessage == MSG_SIZE || (millis() - txTimeout > 100 && toSend == 0)) && digitalRead(settings.irq))
  {
    bytesThisMessage = 0;
    hasTransmission = false;
    radioIdle();
    // Serial.println("finished");
  }

  if (transmit.radio.spiRead(RH_RF69_REG_27_IRQFLAGS1) & RH_RF69_IRQFLAGS1_MODEREADY)
  {
    modeReady = true;
  }

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
  }
}

// adapted from radioHead functions
void radioIdle()
{
  modeReady = false;
  transmit.radio.spiWrite(RH_RF69_REG_5A_TESTPA1, RH_RF69_TESTPA1_NORMAL);
  transmit.radio.spiWrite(RH_RF69_REG_5C_TESTPA2, RH_RF69_TESTPA2_NORMAL);
  uint8_t opmode = transmit.radio.spiRead(RH_RF69_REG_01_OPMODE);
  opmode &= ~RH_RF69_OPMODE_MODE;
  opmode |= (RH_RF69_OPMODE_MODE_STDBY & RH_RF69_OPMODE_MODE);
  transmit.radio.spiWrite(RH_RF69_REG_01_OPMODE, opmode);
}

void radioTx()
{
  modeReady = false;
  transmit.radio.spiWrite(RH_RF69_REG_5A_TESTPA1, RH_RF69_TESTPA1_BOOST);
  transmit.radio.spiWrite(RH_RF69_REG_5C_TESTPA2, RH_RF69_TESTPA2_BOOST);
  transmit.radio.spiWrite(RH_RF69_REG_25_DIOMAPPING1, RH_RF69_DIOMAPPING1_DIO0MAPPING_00); // Set interrupt line 0 PacketSent
  uint8_t opmode = transmit.radio.spiRead(RH_RF69_REG_01_OPMODE);
  opmode &= ~RH_RF69_OPMODE_MODE;
  opmode |= (RH_RF69_OPMODE_MODE_TX & RH_RF69_OPMODE_MODE);
  transmit.radio.spiWrite(RH_RF69_REG_01_OPMODE, opmode);
}
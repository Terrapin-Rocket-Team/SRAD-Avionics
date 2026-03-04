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
#include <string.h>
// create the SPI class to input into the radio
SPIClass Radio_SPI(RADIO_MOSI, RADIO_MISO, RADIO_SCK);
Type2GT radio(RADIO_NCS, RADIO_IO9, RADIO_NRST, RADIO_BUSY, Radio_SPI);

static const size_t MAX_RADIO_LINE = 220;
static const uint32_t ACK_TIMEOUT_MS = 450;
static const uint8_t MAX_ATTEMPTS = 4;

struct PendingMsg
{
  bool active;
  bool waitingAck;
  uint16_t seq;
  uint8_t attempts;
  uint32_t retryAtMs;
  char payload[MAX_RADIO_LINE];
};

PendingMsg pending = {false, false, 0, 0, 0, {0}};
uint16_t txSeq = 0;
uint16_t ackSeqQueued = 0;
uint16_t lastRxSeq = 0;
bool ackQueued = false;
bool hasLastRxSeq = false;

static bool startsWith(const char *str, const char *prefix)
{
  return strncmp(str, prefix, strlen(prefix)) == 0;
}

static void trimLine(char *line)
{
  size_t len = strlen(line);
  while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
  {
    line[len - 1] = '\0';
    len--;
  }
}

static void queueMessage(const char *payload)
{
  if (pending.active)
  {
    Serial.println("RAD/Warn: previous TX awaiting ACK, dropping new line");
    return;
  }

  pending.active = true;
  pending.waitingAck = false;
  pending.seq = ++txSeq;
  pending.attempts = 0;
  pending.retryAtMs = millis();
  strncpy(pending.payload, payload, sizeof(pending.payload) - 1);
  pending.payload[sizeof(pending.payload) - 1] = '\0';
}

static void maybeSendAck()
{
  if (!ackQueued || radio.isBusy())
  {
    return;
  }

  char frame[32];
  snprintf(frame, sizeof(frame), "RAD/ACK,%u", (unsigned)ackSeqQueued);
  int rc = radio.transmit(frame);
  if (rc == RADIOLIB_ERR_NONE)
  {
    ackQueued = false;
  }
}

static void maybeSendMessage()
{
  if (!pending.active || pending.waitingAck || radio.isBusy())
  {
    return;
  }

  if ((int32_t)(millis() - pending.retryAtMs) < 0)
  {
    return;
  }

  if (pending.attempts >= MAX_ATTEMPTS)
  {
    Serial.printf("RAD/Error: seq=%u failed after %u attempts\n", (unsigned)pending.seq, (unsigned)pending.attempts);
    pending.active = false;
    return;
  }

  char frame[320];
  snprintf(frame, sizeof(frame), "RAD/MSG,%u,%s", (unsigned)pending.seq, pending.payload);
  int rc = radio.transmit(frame);
  if (rc == RADIOLIB_ERR_NONE)
  {
    pending.attempts++;
    pending.waitingAck = true;
    pending.retryAtMs = millis() + ACK_TIMEOUT_MS;
    Serial.printf("RAD/Info: TX seq=%u attempt=%u\n", (unsigned)pending.seq, (unsigned)pending.attempts);
  }
}

static void handleAckFrame(const char *frame)
{
  unsigned int ackSeq = 0;
  if (sscanf(frame, "RAD/ACK,%u", &ackSeq) != 1)
  {
    return;
  }

  Serial.printf("RAD/RX ACK,%u\n", ackSeq);
  if (pending.active && pending.waitingAck && pending.seq == (uint16_t)ackSeq)
  {
    pending.active = false;
    pending.waitingAck = false;
    Serial.printf("RAD/Info: ACK matched seq=%u\n", ackSeq);
  }
}

static void handleMsgFrame(const char *frame)
{
  unsigned int rxSeq = 0;
  int payloadOffset = 0;
  if (sscanf(frame, "RAD/MSG,%u,%n", &rxSeq, &payloadOffset) != 1 || payloadOffset <= 0)
  {
    return;
  }

  const char *payload = frame + payloadOffset;
  const bool duplicate = hasLastRxSeq && lastRxSeq == (uint16_t)rxSeq;
  if (!duplicate)
  {
    lastRxSeq = (uint16_t)rxSeq;
    hasLastRxSeq = true;
    Serial.printf("RAD/RX MSG,%u,%s\n", rxSeq, payload);
  }

  ackSeqQueued = (uint16_t)rxSeq;
  ackQueued = true;
}

static void handleRxFrame(const char *frame)
{
  if (startsWith(frame, "RAD/ACK,"))
  {
    handleAckFrame(frame);
  }
  else if (startsWith(frame, "RAD/MSG,"))
  {
    handleMsgFrame(frame);
  }
  else
  {
    Serial.printf("RAD/RX RAW,%s\n", frame);
  }
}

void radInt(void)
{
  radio.handleIrq();
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
  Serial.println("RAD/Info: Transceiver ready");
}

void loop()
{
  RAD_EVENT event = RAD_EVENT_NONE;
  while (radio.popEvent(event))
  {
    if (event == RAD_EVENT_TX_DONE)
    {
      Serial.printf("RAD/Info: TX done in %.2f ms\n", radio.lastTxDurationUs() / 1000.0f);
      radio.recieve();
    }
    else if (event == RAD_EVENT_RX_DONE)
    {
      char rxBuf[320];
      int rxLen = radio.readData(rxBuf, sizeof(rxBuf));
      if (rxLen >= 0)
      {
        handleRxFrame(rxBuf);
      }
      else
      {
        Serial.printf("RAD/Warn: readData failed rc=%d\n", rxLen);
      }
      radio.recieve();
    }
  }

  if (pending.active && pending.waitingAck && (int32_t)(millis() - pending.retryAtMs) >= 0)
  {
    pending.waitingAck = false;
    pending.retryAtMs = millis();
    Serial.printf("RAD/Warn: ACK timeout for seq=%u, retrying\n", (unsigned)pending.seq);
  }

  maybeSendAck();
  maybeSendMessage();

  while (Serial.available())
  {
    char buf[MAX_RADIO_LINE];
    int i = Serial.readBytesUntil('\n', buf, sizeof(buf) - 1);
    if (i <= 0)
    {
      break;
    }
    buf[i] = '\0';
    trimLine(buf);
    if (strlen(buf) == 0)
    {
      continue;
    }

    if (!strncmp(buf, "RAD/PING", 8))
    {
      Serial.println("RAD/PONG");
    }
    else if (!strncmp(buf, "RAD/STATUS", 10))
    {
      Serial.printf("RAD/Status active=%d waitingAck=%d attempts=%u ackQueued=%d busy=%d\n",
                    pending.active ? 1 : 0,
                    pending.waitingAck ? 1 : 0,
                    (unsigned)pending.attempts,
                    ackQueued ? 1 : 0,
                    radio.isBusy() ? 1 : 0);
    }
    else
    {
      queueMessage(buf);
    }
  }

  digitalWrite(STATUS_LED, pending.active || ackQueued ? HIGH : LOW);
}
#endif
/*
  Dont worry about this code, it is just a USB to serial bridge

*/
#ifdef TEENSY

void setup()
{
  Serial.begin(115200);
  Serial1.begin(115200);
}

void loop()
{
  // Still pass through any serial data for commands
  while (Serial1.available())
  {
    Serial.write((char)Serial1.read());
  }

  while (Serial.available())
  {
    Serial1.write((char)Serial.read());
  }
}

#endif

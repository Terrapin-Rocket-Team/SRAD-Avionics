#include <Arduino.h>
#include "RadioMessage.h"
#include "Si4463.h"

#define BUZZER_PIN 0

#define TELEM_DEVICE_ID 3

Si4463HardwareConfig hwcfg = {
    MOD_2GFSK,       // modulation
    DR_40k,          // data rate
    (uint32_t)433e6, // frequency (Hz)
    127,             // tx power (127 = ~20dBm)
    48,              // preamble length
    16,              // required received valid preamble
};

Si4463PinConfig pincfg = {
    &SPI, // spi bus to use
    8,    // cs
    6,    // sdn
    7,    // irq
    9,    // gpio0
    10,   // gpio1
    4,    // random pin - gpio2 is not connected
    5,    // random pin - gpio3 is not connected
};

Si4463 radio(hwcfg, pincfg);

enum InputState
{
  HANDSHAKE,
  COMMAND,
  // add additional states as necessary
  NONE
};

// overall behavior control
bool handshakeSuccess = false;
bool hasDataHeader = false;

// Serial handling control
InputState currState = NONE;

// Serial communication handling
bool foundNewline = false;
int bytesAvail = 0;
char serialBuf[Message::maxSize] = {0};
int serialBufLength = 0;
Message commandMsg;
APRSConfig commandConfig = {"KC3UTM", "ALL", "WIDE1-1", PositionWithoutTimestampWithoutAPRS, '\\', 'M'};
uint16_t commandSize = 0;

// data handling
uint32_t timerMetrics = millis();
Message m;
APRSTelem avionicsTelem;
GSData avionicsData(APRSTelem::type, 1, TELEM_DEVICE_ID);
bool hasAvionicsTelem = false;

// sample metrics implementation
Message metricsMessage;
Metrics telemMetrics(TELEM_DEVICE_ID);
GSData metricsGSData(Metrics::type, 4, TELEM_DEVICE_ID);

void beep(int d)
{
  digitalWrite(BUZZER_PIN, HIGH);
  delay(d);
  digitalWrite(BUZZER_PIN, LOW);
  delay(d);
}

void setup()
{
  // Modify baud rate to match desired bitrate
  Serial.begin(115200);
  // setup buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  if (CrashReport)
  {
    Serial.println(CrashReport);
    while (1)
      beep(100);
  }

  if (!radio.begin())
  {
    // Serial.println("Error: radio failed to begin");
    // Serial.flush();
    beep(1000);
    while (1)
      ;
  }

  beep(100);
}

void loop()
{
  if (((bytesAvail = Serial.available()) > 0))
  {
    // check if a command is being sent
    if (currState == NONE)
    {
      for (int i = 0; i < bytesAvail; i++)
      {
        char c = Serial.read();
        if (c == '\n')
        {
          serialBuf[i] = 0;
          foundNewline = true;
          break;
        }
        serialBuf[i] = c;
        serialBufLength++;
        if (serialBufLength >= (int)sizeof(serialBuf))
          break;
      }
      bytesAvail = Serial.available();

      if (foundNewline)
      {
        if (strcmp(serialBuf, "handshake") == 0)
        {
          // begin the handshake
          handshakeSuccess = false;
          currState = HANDSHAKE;
        }
        else if (strcmp(serialBuf, "handshake succeeded") == 0)
        {
          // the handshake was successful
          handshakeSuccess = true;
          // we will start sending data, so setup metrics
          telemMetrics.setInitialTime(millis());
        }
        else if (strcmp(serialBuf, "handshake failed") == 0)
        {
          // the handshake was not successful
          handshakeSuccess = false;
        }
        else if (strcmp(serialBuf, "command") == 0)
        {
          // the next text will be a radio command
          commandMsg.clear();
          currState = COMMAND;
        }
        // add additional commands as required

        // reset serial buffer
        memset(serialBuf, 0, sizeof(serialBuf));
        foundNewline = false;
      }
    }

    // complete the handshake
    if (currState == HANDSHAKE)
    {
      for (int i = 0; i < bytesAvail; i++)
      {
        char c = Serial.read();
        // skip odd characters that accidently get added
        if (c != 0xff && c != 0)
        {
          Serial.write(c);
          if (c == '\n')
          {
            foundNewline = true;
            break;
          }
        }
      }
      bytesAvail = Serial.available();

      if (foundNewline)
      {
        foundNewline = false;
        currState = NONE;
      }
    }

    // receive the radio command
    if (currState == COMMAND)
    {
      // read into serial buffer
      for (int i = 0; i < bytesAvail; i++)
      {
        commandMsg.append(Serial.read());
        if (commandMsg.size >= Message::maxSize)
          break;
      }
      bytesAvail = Serial.available();

      if (commandMsg.size >= GSData::headerLen)
      {
        // we can decode the header
        uint8_t type, id, deviceId = 0;
        GSData::decodeHeader(commandMsg.buf, type, id, deviceId, commandSize);
        if (type != APRSCmd::type || id == 0 || commandSize == 0)
        {
          // this is not an APRSCmd, ignore it
          currState = NONE;
        }
      }

      if (commandMsg.size - GSData::headerLen > commandSize && commandSize != 0)
      {
        // remove the extra bytes and put them in the serial buffer in case they are part of a different message
        uint16_t removed = (commandMsg.size - GSData::headerLen) - commandSize;
        commandMsg.pop((uint8_t *)serialBuf, removed);
        serialBufLength += removed;
      }
      if (commandMsg.size - GSData::headerLen == commandSize && commandSize != 0)
      {
        // we have the full command, so decode it
        GSData multiplexedCommand;
        commandMsg.decode(&multiplexedCommand);
        // send the actual command data
        commandMsg.clear();
        commandMsg.fill(multiplexedCommand.buf, multiplexedCommand.size);
        APRSCmd cmd;
        commandMsg.decode(&cmd);
        cmd.config = commandConfig;
        commandMsg.encode(&cmd);
        commandMsg.write(Serial); // TODO: send to radio
        // we are now finished handling the radio command on the Serial side
        currState = NONE;
      }
    }

    // add additional command handling here
  }

  // radio
  if (handshakeSuccess && radio.avail())
  {
    // get the message
    radio.receive(avionicsTelem);
    // re-encode it to be multiplexed
    m.encode(&avionicsTelem);
    // update metrics
    telemMetrics.update(m.size, millis(), radio.RSSI());
    // set the flag to transmit data
    hasAvionicsTelem = true;
  }

  if (handshakeSuccess)
  {
    // output goes here
    if (hasAvionicsTelem)
    {
      // fill GSData with message
      avionicsData.fill(m.buf, m.size);
      // encode for multplexing
      m.encode(&avionicsData);
      // write
      Serial.write(m.buf, m.size);
      // reset flag
      hasAvionicsTelem = false;
    }

    if (millis() - timerMetrics > 1000)
    {
      timerMetrics = millis();
      metricsMessage.encode(&telemMetrics);
      metricsGSData.fill(metricsMessage.buf, metricsMessage.size);
      metricsMessage.encode(&metricsGSData);
      Serial.write(metricsMessage.buf, metricsMessage.size);
    }
  }

  radio.update();
}
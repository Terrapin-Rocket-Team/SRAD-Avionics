#include <Arduino.h>
#include "RadioMessage.h"
#include "Si4463.h"

// #define BUZZER_PIN 0

#define TELEM_DEVICE_ID 3

const char avionicsCall[] = "KD3BBD";
const char airbrakeCall[] = "KC3UTM";
const char payloadCall[] = "KQ4TCN";
const char commandCall[] = "KD3BBD"; // TODO

Si4463HardwareConfig hwcfg = {
    MOD_2GFSK,       // modulation
    DR_100k,         // data rate
    (uint32_t)433e6, // frequency (Hz)
    127,             // tx power (127 = ~20dBm)
    48,              // preamble length
    16,              // required received valid preamble
};

Si4463PinConfig pincfg = {
    &SPI, // spi bus to use
    10,   // cs
    38,   // sdn
    33,   // irq
    34,   // gpio0
    35,   // gpio1
    36,   // random pin - gpio2 is not connected
    37,   // random pin - gpio3 is not connected
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
APRSConfig commandConfig = {"KD3BBD", "ALL", "WIDE1-1", PositionWithoutTimestampWithoutAPRS, '\\', 'M'};
uint16_t commandSize = 0;

// data handling
uint32_t timerMetrics = millis();
Message m;

APRSTelem telem;
GSData avionicsData(APRSTelem::type, 1, TELEM_DEVICE_ID);
bool hasAvionicsTelem = false;
GSData airbrakeData(APRSTelem::type, 2, TELEM_DEVICE_ID);
bool hasAirbrakeTelem = false;
GSData payloadData(APRSTelem::type, 3, TELEM_DEVICE_ID);
bool hasPayloadTelem = false;

// sample metrics implementation
Message metricsMessage;
Metrics telemMetrics(TELEM_DEVICE_ID);
GSData metricsGSData(Metrics::type, 4, TELEM_DEVICE_ID);

// void beep(int d)
// {
//   digitalWrite(BUZZER_PIN, HIGH);
//   delay(d);
//   digitalWrite(BUZZER_PIN, LOW);
//   delay(d);
// }

void log(const char *str1, const char *str2 = "", const char *str3 = "")
{
  Serial.print(str1);
  Serial.print(str2);
  Serial.print(str3);
  Serial.write("\n");
}

void setup()
{
  // Modify baud rate to match desired bitrate
  Serial.begin(115200);
  Serial5.begin(115200);
  // setup buzzer
  // pinMode(BUZZER_PIN, OUTPUT);
  // digitalWrite(BUZZER_PIN, LOW);

  if (CrashReport)
  {
    Serial.println(CrashReport);
    while (1)
      ;
    // beep(100);
  }

  // if (!radio.begin())
  // {
  //   log("Error: radio failed to begin");
  //   beep(1000);
  //   while (1)
  //     ;
  // }

  log("Initialization complete");
  log("Avionics callsign is: ", avionicsCall);
  log("Airbrake callsign is: ", airbrakeCall);
  log("Payload callsign is: ", payloadCall);
  log("Command calsign is: ", commandCall);

  // while (1)
  // {
  //   Serial5.println("hello world!");
  //   while (Serial5.available() > 0)
  //     Serial5.write(Serial5.read());
  //   delay(1000);
  // }
  // beep(100);
}

void loop()
{
  if (((bytesAvail = Serial5.available()) > 0))
  {
    // check if a command is being sent
    if (currState == NONE)
    {
      log("buf");
      for (int i = 0; i < bytesAvail; i++)
      {
        char c = Serial5.read();
        if (c == '\n')
        {
          serialBuf[serialBufLength] = 0;
          foundNewline = true;
          break;
        }
        serialBuf[serialBufLength] = c;
        serialBufLength++;
        if (serialBufLength >= (int)sizeof(serialBuf))
          break;
      }
      bytesAvail = Serial5.available();

      if (foundNewline)
      {
        log("newline command");
        if (strcmp(serialBuf, "handshake") == 0)
        {
          log("Starting handshake");
          // begin the handshake
          handshakeSuccess = false;
          currState = HANDSHAKE;
        }
        else if (strcmp(serialBuf, "handshake succeeded") == 0)
        {
          log("Successful handshake");
          // the handshake was successful
          handshakeSuccess = true;
          // we will start sending data, so set up metrics
          telemMetrics.setInitialTime(millis());
        }
        else if (strcmp(serialBuf, "handshake failed") == 0)
        {
          log("Failed handshake");
          // the handshake was not successful
          handshakeSuccess = false;
        }
        else if (strcmp(serialBuf, "command") == 0)
        {
          log("Ready for radio command");
          // the next text will be a radio command
          commandMsg.clear();
          currState = COMMAND;
        }
        // add additional commands as required

        // reset serial buffer
        memset(serialBuf, 0, sizeof(serialBuf));
        serialBufLength = 0;
        foundNewline = false;
      }
    }

    // complete the handshake
    if (currState == HANDSHAKE)
    {
      for (int i = 0; i < bytesAvail; i++)
      {
        char c = Serial5.read();
        log("read:");
        // skip odd characters that accidently get added
        if (c != 0xff && c != 0)
        {
          Serial5.write(c);
          if (c == '\n')
          {
            log("newline handshake");
            foundNewline = true;
            break;
          }
        }
      }
      bytesAvail = Serial5.available();

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
        commandMsg.append(Serial5.read());
        if (commandMsg.size >= Message::maxSize)
          break;
      }
      bytesAvail = Serial5.available();

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
        commandMsg.write(Serial); // TODO: log for now, need to send to radio
        // we are now finished handling the radio command on the Serial side
        currState = NONE;
      }
    }

    // add additional command handling here
  }

  // radio
  // if (handshakeSuccess && radio.avail())
  // {
  //   // get the message
  //   radio.receive(telem);
  //   // re-encode it to be multiplexed
  //   m.encode(&telem);
  //   // update metrics
  //   telemMetrics.update(m.size, millis(), radio.RSSI());
  //   // set the flag to transmit data
  //   if (strcmp(telem.config.callsign, avionicsCall) == 0)
  //     hasAvionicsTelem = true;
  //   if (strcmp(telem.config.callsign, airbrakeCall) == 0)
  //     hasAirbrakeTelem = true;
  //   if (strcmp(telem.config.callsign, payloadCall) == 0)
  //     hasPayloadTelem = true;
  // }

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
      log("Avionics data: ", (const char *)m.buf);
      Serial5.write(m.buf, m.size);
      // reset flag
      hasAvionicsTelem = false;
    }

    if (hasAirbrakeTelem)
    {
      // fill GSData with message
      airbrakeData.fill(m.buf, m.size);
      // encode for multplexing
      m.encode(&airbrakeData);
      // write
      log("Airbrake data: ", (const char *)m.buf);
      Serial5.write(m.buf, m.size);
      // reset flag
      hasAirbrakeTelem = false;
    }

    if (hasPayloadTelem)
    {
      // fill GSData with message
      payloadData.fill(m.buf, m.size);
      // encode for multplexing
      m.encode(&payloadData);
      // write
      log("Payload data: ", (const char *)m.buf);
      Serial5.write(m.buf, m.size);
      // reset flag
      hasPayloadTelem = false;
    }

    if (millis() - timerMetrics > 1000)
    {
      timerMetrics = millis();
      metricsMessage.encode(&telemMetrics);
      metricsGSData.fill(metricsMessage.buf, metricsMessage.size);
      metricsMessage.encode(&metricsGSData);
      Serial5.write(metricsMessage.buf, metricsMessage.size);
    }
  }

  // radio.update();
}
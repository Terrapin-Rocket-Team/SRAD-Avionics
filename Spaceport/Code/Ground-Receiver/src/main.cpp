#include <Arduino.h>
#include "RadioMessage.h"
#include "Si4463.h"

#include "422Mc86_4GFSK_500000H.h"
#include "422Mc80_4GFSK_010000H.h"

#define TELEM_DEVICE_ID 3
#define AVIONICS_DEVICE_ID 2
#define PAYLOAD_DEVICE_ID 1

const char avionicsCall[] = "KD3BBD";
const char airbrakeCall[] = "KC3UTM";
const char payloadCall[] = "KQ4TCN";
const char commandCall[] = "KD3BBD"; // TODO: sync with commandConfig object

HardwareSerial *s;

Si4463HardwareConfig hwcfgTelem = {
    MOD_4GFSK,       // modulation
    DR_9_6k,         // data rate
    (uint32_t)430e6, // frequency (Hz)
    POWER_HP_33dBm,  // tx power (127 = ~20dBm)
    192,             // preamble length
    32,              // required received valid preamble
};

Si4463PinConfig pincfgTelem = {
    &SPI, // spi bus to use
    33,   // cs
    39,   // sdn
    34,   // irq
    35,   // gpio0
    36,   // gpio1
    37,   // gpio2
    38,   // gpio3
};

Si4463HardwareConfig hwcfgAvionics = {
    MOD_4GFSK,       // modulation
    DR_250k,         // data rate
    (uint32_t)433e6, // frequency (Hz)
    // (uint32_t)430.6e6, // frequency (Hz)
    POWER_HP_20dBm, // tx power (127 = ~20dBm)
    192,            // preamble length
    32,             // required received valid preamble
};

Si4463PinConfig pincfgAvionics = {
    &SPI, // spi bus to use
    30,   // cs
    29,   // sdn
    24,   // irq
    25,   // gpio0
    26,   // gpio1
    27,   // gpio2
    28,   // gpio3
};

Si4463HardwareConfig hwcfgPayload = {
    MOD_4GFSK,         // modulation
    DR_250k,           // data rate
    (uint32_t)431.3e6, // frequency (Hz)
    POWER_HP_20dBm,    // tx power (127 = ~20dBm)
    192,               // preamble length
    32,                // required received valid preamble
};

Si4463PinConfig pincfgPayload = {
    &SPI, // spi bus to use
    6,    // cs
    5,    // sdn
    0,    // irq
    1,    // gpio0
    2,    // gpio1
    3,    // gpio2
    4,    // gpio3
};

Si4463 radioTelem(hwcfgTelem, pincfgTelem);
Si4463 radioAvionics(hwcfgAvionics, pincfgAvionics);
Si4463 radioPayload(hwcfgPayload, pincfgPayload);

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

// telemetry
APRSTelem telem;
GSData avionicsData(APRSTelem::type, 1, TELEM_DEVICE_ID);
bool hasAvionicsTelem = false;
GSData airbrakeData(APRSTelem::type, 2, TELEM_DEVICE_ID);
bool hasAirbrakeTelem = false;
GSData payloadData(APRSTelem::type, 3, TELEM_DEVICE_ID);
bool hasPayloadTelem = false;

// video
Message avionicsVideoMessage;
VideoData avionicsVideo;
GSData avionicsVideoData(VideoData::type, 5, AVIONICS_DEVICE_ID);
bool hasAvionicsVideo = false;
Message payloadVideoMessage;
VideoData payloadVideo;
GSData payloadVideoData(VideoData::type, 6, PAYLOAD_DEVICE_ID);
bool hasPayloadVideo = false;

// sample metrics implementation
Message metricsMessage;
Metrics telemMetrics(TELEM_DEVICE_ID);
Metrics avionicsVideoMetrics(AVIONICS_DEVICE_ID);
Metrics payloadVideoMetrics(PAYLOAD_DEVICE_ID);
GSData metricsGSData(Metrics::type, 4, TELEM_DEVICE_ID);

void log(const char *str1, const char *str2 = "", const char *str3 = "")
{
  Serial5.print(str1);
  Serial5.print(str2);
  Serial5.print(str3);
  Serial5.write("\n");
}

void setup()
{
  // Modify baud rate to match desired bitrate
  Serial.begin(1000000);
  Serial5.begin(115200);

  s = (HardwareSerial *)&Serial;

  if (CrashReport)
  {
    Serial5.println(CrashReport);
    while (1)
      ;
  }

  // if (!radioTelem.begin(CONFIG_422Mc80_4GFSK_010000H, sizeof(CONFIG_422Mc80_4GFSK_010000H)))
  // {
  //   log("Error: telemetry radio failed to begin");
  //   while (1)
  //     ;
  // }

  if (!radioAvionics.begin(CONFIG_422Mc86_4GFSK_500000H, sizeof(CONFIG_422Mc86_4GFSK_500000H)))
  {
    log("Error: Avionics video radio failed to begin");
    while (1)
      ;
  }

  // if (!radioPayload.begin(CONFIG_422Mc86_4GFSK_500000H, sizeof(CONFIG_422Mc86_4GFSK_500000H)))
  // {
  //   log("Error: Payload video radio failed to begin");
  //   while (1)
  //     ;
  // }

  log("Initialization complete");
  log("Avionics callsign is: ", avionicsCall);
  log("Airbrake callsign is: ", airbrakeCall);
  log("Payload callsign is: ", payloadCall);
  log("Command calsign is: ", commandCall);
}

void loop()
{
  if (((bytesAvail = s->available()) > 0))
  {
    // check if a command is being sent
    if (currState == NONE)
    {
      log("buf");
      for (int i = 0; i < bytesAvail; i++)
      {
        char c = s->read();
        if (c == '\n')
        {
          serialBuf[serialBufLength] = 0;
          foundNewline = true;
          break;
        }
        if (c != '\0')
        {
          serialBuf[serialBufLength] = c;
          serialBufLength++;
          if (serialBufLength >= (int)sizeof(serialBuf))
            break;
        }
      }
      bytesAvail = s->available();

      if (foundNewline)
      {
        log("newline command ", serialBuf);
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
        char c = s->read();
        log("read:");
        // skip odd characters that accidently get added
        if (c != 0xff && c != 0)
        {
          s->write(c);
          if (c == '\n')
          {
            log("newline handshake");
            foundNewline = true;
            break;
          }
        }
      }
      bytesAvail = s->available();

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
        commandMsg.append(s->read());
        if (commandMsg.size >= Message::maxSize)
          break;
      }
      bytesAvail = s->available();

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
  // if (handshakeSuccess && radioTelem.avail())
  // {
  //   // get the message
  //   radioTelem.receive(telem);
  //   // re-encode it to be multiplexed
  //   m.encode(&telem);
  //   // update metrics
  //   telemMetrics.update(m.size, millis(), radioTelem.RSSI());
  //   // set the flag to transmit data
  //   if (strcmp(telem.config.callsign, avionicsCall) == 0)
  //     hasAvionicsTelem = true;
  //   if (strcmp(telem.config.callsign, airbrakeCall) == 0)
  //     hasAirbrakeTelem = true;
  //   if (strcmp(telem.config.callsign, payloadCall) == 0)
  //     hasPayloadTelem = true;
  // }

  if (handshakeSuccess && radioAvionics.avail())
  {
    // get the message
    avionicsVideo.size = radioAvionics.readRXBuf(avionicsVideo.data, radioAvionics.length);
    // re-encode it to be multiplexed
    avionicsVideoMessage.encode(&avionicsVideo);
    // update metrics
    avionicsVideoMetrics.update(avionicsVideoMessage.size, millis(), radioAvionics.RSSI());
    // set the flag to transmit data
    hasAvionicsVideo = true;
    // reset avail flag
    radioAvionics.available = false;
  }

  // if (handshakeSuccess && radioPayload.avail())
  // {
  //   // get the message
  //   payloadVideo.size = radioPayload.readRXBuf(payloadVideo.data, VideoData::maxSize);
  //   // re-encode it to be multiplexed
  //   payloadVideoMessage.encode(&payloadVideo);
  //   // update metrics
  //   payloadVideoMetrics.update(payloadVideoMessage.size, millis(), radioPayload.RSSI());
  //   // set the flag to transmit data
  //   hasPayloadVideo = true;
  //   // reset avail flag
  //   radioPayload.available = false
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
      s->write(m.buf, m.size);
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
      s->write(m.buf, m.size);
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
      s->write(m.buf, m.size);
      // reset flag
      hasPayloadTelem = false;
    }

    if (hasAvionicsVideo)
    {
      if (avionicsVideoMessage.size > GSData::maxSize)
      {
        // Assume it is no more than double the maxSize
        // Need to split into 255 bytes chunks (assume it is an integer multiple of 255)
        int chunks = avionicsVideo.size / 255;
        int chunksFirstHalf = chunks - chunks / 2;
        int chunksSecondHalf = chunks - chunksFirstHalf;

        // fill GSData with first half of message
        avionicsVideoData.fill(avionicsVideo.data, chunksFirstHalf * 255);
        // encode for multplexing
        avionicsVideoMessage.encode(&avionicsVideoData);
        // write
        s->write(avionicsVideoMessage.buf, avionicsVideoMessage.size);
        // fill GSData with second half of message
        avionicsVideoData.fill(avionicsVideo.data + (chunksFirstHalf * 255), chunksSecondHalf * 255);
        // encode for multplexing
        avionicsVideoMessage.encode(&avionicsVideoData);
        // write
        s->write(avionicsVideoMessage.buf, avionicsVideoMessage.size);
        // reset flag
        hasAvionicsVideo = false;
      }
      else
      {
        // fill GSData with message
        avionicsVideoData.fill(avionicsVideoMessage.buf, avionicsVideoMessage.size);
        // encode for multplexing
        avionicsVideoMessage.encode(&avionicsVideoData);
        // write
        s->write(avionicsVideoMessage.buf, avionicsVideoMessage.size);
        // reset flag
        hasAvionicsVideo = false;
      }
    }

    if (hasPayloadVideo)
    {
      if (payloadVideoMessage.size > GSData::maxSize)
      {
        // Assume it is no more than double the maxSize
        // Need to split into 255 bytes chunks (assume it is an integer multiple of 255)
        int chunks = payloadVideoMessage.size / 255;
        int chunksFirstHalf = chunks - chunks / 2;
        int chunksSecondHalf = chunks - chunksFirstHalf;

        // fill GSData with first half of message
        payloadVideoData.fill(payloadVideoMessage.buf, chunksFirstHalf * 255);
        // encode for multplexing
        payloadVideoMessage.encode(&payloadVideoData);
        // write
        s->write(payloadVideoMessage.buf, payloadVideoMessage.size);
        // fill GSData with second half of message
        payloadVideoData.fill(payloadVideoMessage.buf + (chunksFirstHalf * 255), chunksSecondHalf * 255);
        // encode for multplexing
        payloadVideoMessage.encode(&payloadVideoData);
        // write
        s->write(payloadVideoMessage.buf, payloadVideoMessage.size);
        // reset flag
        hasPayloadVideo = false;
      }
      else
      {
        // fill GSData with message
        payloadVideoData.fill(payloadVideoMessage.buf, payloadVideoMessage.size);
        // encode for multplexing
        payloadVideoMessage.encode(&payloadVideoData);
        // write
        s->write(payloadVideoMessage.buf, payloadVideoMessage.size);
        // reset flag
        hasPayloadVideo = false;
      }
    }

    if (millis() - timerMetrics > 1000)
    {
      timerMetrics = millis();
      metricsMessage.encode(&telemMetrics);
      metricsGSData.fill(metricsMessage.buf, metricsMessage.size);
      metricsMessage.encode(&metricsGSData);
      s->write(metricsMessage.buf, metricsMessage.size);
      metricsMessage.encode(&avionicsVideoMetrics);
      metricsGSData.fill(metricsMessage.buf, metricsMessage.size);
      metricsMessage.encode(&metricsGSData);
      s->write(metricsMessage.buf, metricsMessage.size);
      metricsMessage.encode(&payloadVideoMetrics);
      metricsGSData.fill(metricsMessage.buf, metricsMessage.size);
      metricsMessage.encode(&metricsGSData);
      s->write(metricsMessage.buf, metricsMessage.size);
    }
  }

  // radioTelem.update();
  radioAvionics.update();
  // radioPayload.update();
}
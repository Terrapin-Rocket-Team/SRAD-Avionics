#include <Arduino.h>
#include "RadioMessage.h"
#include "Si4463.h"
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

static const unsigned char PROGMEM logo_bmp[] =
{ 0b00000000, 0b11000000,
  0b00000001, 0b11000000,
  0b00000001, 0b11000000,
  0b00000011, 0b11100000,
  0b11110011, 0b11100000,
  0b11111110, 0b11111000,
  0b01111110, 0b11111111,
  0b00110011, 0b10011111,
  0b00011111, 0b11111100,
  0b00001101, 0b01110000,
  0b00011011, 0b10100000,
  0b00111111, 0b11100000,
  0b00111111, 0b11110000,
  0b01111100, 0b11110000,
  0b01110000, 0b01110000,
  0b00000000, 0b00110000 };



#define BUZZER_PIN 0

#define TELEM_DEVICE_ID 3

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
APRSConfig commandConfig = {"KC3UTM", "ALL", "WIDE1-1", PositionWithoutTimestampWithoutAPRS, '\\', 'M'};
uint16_t commandSize = 0;

// data handling
uint32_t timerMetrics = millis();
Message m;

APRSTelem telem;
GSData avionicsData(APRSTelem::type, 1, TELEM_DEVICE_ID);
bool hasAvionicsTelem = false;
GSData airbrakeData(APRSTelem::type, 2, TELEM_DEVICE_ID);
bool hasAirbrakeTelem = false;

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
  //adding the ground station OLED stuff
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
  delay(2000);
  // display.display() is NOT necessary after every single drawing command,
  // unless that's what you want...rather, you can batch up a bunch of
  // drawing operations and then update the screen all at once by calling
  // display.display(). These examples demonstrate both approaches...

  // Invert and restore display, pausing in-between
  display.invertDisplay(true);
  delay(1000);
  display.invertDisplay(false);
  delay(1000);

  beep(100);
}

void loop()
{

  //adding the OLED stuff 
  char s[50];


  display.clearDisplay();
  //all the settings for the text 
  display.setCursor(0, 0);
  display.setFont(NULL);
  display.setTextColor(SSD1306_WHITE);
  display.startWrite();
  display.setTextSize(1);
  display.setTextWrap(false);

  //latitude double addition 
  double lf = -0.2;
  snprintf(s, sizeof(s), "Lat: %lf \n", lf);
  display.print(s);

  //longtitude double addition
  double lf2 = -0.5;
  snprintf(s, sizeof(s), "Long: %lf", lf2);
  display.setCursor(0, 8);
  display.print(s);

  //RSSI integer addition
  int lf3 = 35;
  snprintf(s, sizeof(s), "RSSI: %d", lf3);
  display.setCursor(0, 17);
  display.print(s);


  //displaying avionics, airbrake, and payload warning signals
  display.setCursor(24, 25);
  display.print("AVI, AIR, PAY");
  //optional delay
  delay(100);
  //DISPLAY.DISPLAY() IS NEEDED!!
  display.display();

  //optional delay
  delay(2000);


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
    radio.receive(telem);
    // re-encode it to be multiplexed
    m.encode(&telem);
    // update metrics
    telemMetrics.update(m.size, millis(), radio.RSSI());
    // set the flag to transmit data
    if (strcmp(telem.config.callsign, "KC3UTM") == 0)
      hasAirbrakeTelem = true;
    if (strcmp(telem.config.callsign, "KC3YKX") == 0)
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

    if (hasAirbrakeTelem)
    {
      // fill GSData with message
      airbrakeData.fill(m.buf, m.size);
      // encode for multplexing
      m.encode(&airbrakeData);
      // write
      Serial.write(m.buf, m.size);
      // reset flag
      hasAirbrakeTelem = false;
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
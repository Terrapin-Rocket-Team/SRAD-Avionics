#ifndef GSINTERFACE_H
#define GSINTERFACE_H

#include "Arduino.h"
#include "RadioMessage.h"

enum InputState
{
    IS_SLEEP,     // not ready to read or write
    IS_HANDSHAKE, // performing a handshake
    IS_NORMAL,    // ready to read or write
    IS_HITL       // in Hardware In The Loop mode
};

enum LogLevel
{
    LL_DEBUG,
    LL_WARN,
    LL_ERROR,
    LL_FATAL
};

typedef bool (*GSInterface_DataCB)(GSMessage *);
typedef void (*GSInterface_ModeCB)(InputState);

// struct to consolidate information about a single stream
struct GSStream
{
    GSStream(uint8_t type, uint8_t streamId, Metrics *m) : type(type), id(streamId), streamMetrics(m) {}
    // stream type for multiplexing metadata
    uint8_t type;
    // stream id for multiplexing metadata
    uint8_t id;
    // pointer to the Metrics for the stream
    // each metrics could be connected to multiple streams, so it is a pointer
    // references to all pointers are stored in the GSInterface object
    // memory is deleted with the GSInterface object
    Metrics *streamMetrics;
};

class GSInterface
{
public:
    // interface version
    static const char version[];
    // the baud rate to use for the primary interface
    uint32_t baud = 115200;
    // the baud rate to use for the debug interface
    uint32_t debugBaud = 0;
    // the buffer to initially store serial data in
    char serialBuf[Message::maxSize] = {0};
    // the length of serialBuf
    uint16_t serialBufLength = 0;

    // whether the GSInterface object is initialized properly and ready to be used
    bool ready = false;
    // whether a handshake with the ground station has successfully been accomplished
    bool handshake = false;
    // the current state of the interface
    InputState state = IS_SLEEP;

    // the index of the next stream
    // incremented when a new stream is created
    // indexes are assigned in order of ```createStream()``` being called
    // 0 used to indicate error, so start at 1
    uint8_t streamIndex = 1;

    // stores references to all metrics so they can be deleted when the GSInterface is
    Metrics **metricsArr = new Metrics *[0];
    // current number of metrics in array
    uint16_t numMetrics = 0;
    // the interval at which to log metrics in ms
    uint32_t metricsInterval = 1000;
    // the id used for all status out (GSControl, Metrics, etc)
    uint8_t statusId = streamIndex++;
    // the GSMessage object for metrics (always stream index 1)
    GSMessage output = {Metrics::type, statusId};

    // used to store the size of input from the ground station
    uint16_t inputSize = 0;
    // used to decode input from the ground station
    GSMessage input;
    // whether there is input from the ground station
    bool hasInput = false;
    // whether there is input that needs to be manually processed
    bool hasData = false;

    // GSInterface constructor
    // - baud : the type of the message
    // - debugBaud : the multiplexing id of the message
    // - interval : the frequency at which to send Metrics for each stream in ms
    GSInterface(uint32_t baud, uint32_t debugBaud = 0, uint32_t interval = 1000);
    // GSInterface destructor
    ~GSInterface();

    // begin function defined based on platform, add ifdefs for other platforms as required
    // used to abstract serial interface, implementations required for serial private methods
    // default implementation does nothing
    // - available()
    // - write()
    // - writeC()
    // - read()
    // - readC()
    // - time()
    // - reset
#ifdef ARDUINO
    // setup the serial ports ```s``` and ```sd```, being the primary and debug serial ports
    bool begin(HWCDC *s, HWCDC *sd = nullptr);
    // pointer to the primary serial port
    HWCDC *s;
    // pointer to the debug serial port
    HWCDC *sd;
#else
    // default begin function, does nothing
    bool begin();
#endif

    // handles all standard GSInterface tasks that need to run every loop, should run as fast as possible
    // returns whether there is input that needs to be handled manually
    bool run();

    // returns whether the GSInterface is ready
    bool isReady();

    // create a new GSStream of RadioMessage type ```type```, coming from device with id ```deviceId```
    GSStream createStream(uint8_t type, uint8_t deviceId);

    // writes ```data``` to GSStream ```s```, ```signalStrength``` used for Metrics (set to 0 if not applicable)
    // returns the number of bytes written
    int writeStream(GSStream *s, Data *data, short signalStrength = 0);
    // writes ```dataLen``` bytes from ```data``` to GSStream ```s```, ```signalStrength``` used for Metrics (set to 0 if not applicable)
    // returns the number of bytes written
    int writeStream(GSStream *s, char *data, int dataLen, short signalStrength = 0);

    // read up to ```dataLen``` bytes from the serial port into ```data```
    // returns the number of bytes read
    bool readInput(Data *data);

    // set the user defined GSControl handler, does not override the default handler
    void setUserControlHandler(GSControl_CB c);
    // set the user defined Data handler, which handles Data input such as commands
    void setUserDataHandler(GSInterface_DataCB c);
    // set the user defined mode switch handler, which is called when switching to a new mode via GSControl command
    void setUserModeHandler(GSInterface_ModeCB c);

    // clears a previously set user defined GSControl handler
    void clearUserControlHandler();
    // clears a previously set user defined Data handler
    void clearUserDataHandler();
    // clear a previously set user defined mode switch handler
    void clearUserModeHandler();

    // logs to the Ground Station via GSControl command
    void logM(LogLevel lvl, const char *str);
    // log to debug serial port, writes ```str1```, ```str2```, then ```str3```, followed by a newline
    void log(const char *str1, const char *str2 = "", const char *str3 = "");

private:
    // string array to convert the LogLevel enum to strings
    static const char *const logLevelStr[];

    // timer variable for sending metrics at an interval
    uint32_t metricsTimer = 0;
    // timer variable for input timeout, ensures the input does not get stuck waiting for a long message because of a bad header
    uint32_t inputTimer = 0;

    // the default GSControl handler
    bool defaultControlHandler(char *cmd, uint16_t argc, char **argv);
    // pointer for user defined GSControl handler
    GSControl_CB userControlHandler = nullptr;
    // pointer for user defined Data handler
    GSInterface_DataCB userDataHandler = nullptr;
    // pointer for user defined mode switch handler
    GSInterface_ModeCB userModeHandler = nullptr;

    // returns the number of bytes available from the serial port
    int available();
    // write ```length``` bytes from ```s``` to the serial port
    // returns the number of bytes written
    int write(char *s, uint32_t length);
    // write ```c``` to the serial port
    void writeC(char c);
    // read up to ```length``` bytes into ```s``` from the serial port
    // returns the number of bytes read
    int read(char *s, uint32_t length);
    // returns a single byte read from the serial port
    char readC();
    // returns the current time since power on in milliseconds
    uint32_t time();
    // performs a hard reset of the device
    void reset();
};

#endif
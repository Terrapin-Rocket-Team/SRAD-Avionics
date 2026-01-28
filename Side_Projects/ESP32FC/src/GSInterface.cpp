#include "GSInterface.h"

const char GSInterface::version[] = "v2.1.0";
const char *const GSInterface::logLevelStr[] = {"DEBUG", "WARN", "ERROR", "FATAL"};

GSInterface::GSInterface(uint32_t baud, uint32_t debugBaud, uint32_t interval) : baud(baud), debugBaud(debugBaud), metricsInterval(interval) {}

GSInterface::~GSInterface()
{
    // delete each Metrics pointer
    for (int i = 0; i < this->numMetrics; i++)
        delete this->metricsArr[i];
    delete[] this->metricsArr;
}

#ifdef ARDUINO
bool GSInterface::begin(HWCDC *serial, HWCDC *serialDebug)
{
    // make sure serial is not a nullptr, then set up and set the baud rate
    if (serial != nullptr)
    {
        this->s = serial;
        this->s->begin(this->baud);
        // flush input buffer
        while (this->s->available())
            this->s->read();
    }

    // make sure serialDebug is not a nullptr, a baud rate was given, then set up and set the baud rate
    if (serialDebug != nullptr && this->debugBaud > 0)
    {
        this->sd = serialDebug;
        this->sd->begin(this->debugBaud);
        // flush input buffer
        while (this->sd->available())
            this->sd->read();
    }

    // check that the main serial is not a null pointer
    // and that the serialDebug was either not given (a null pointer)
    // or was given and the debug baud rate was set
    this->ready = this->s != nullptr && (sd == nullptr || (sd != nullptr && this->sd != nullptr && this->debugBaud > 0));

    // if the interface it ready enter normal state
    if (this->ready)
        this->state = IS_NORMAL;

    return this->ready;
}
#else
bool GSInterface::begin() { return false; }
#endif

bool GSInterface::run()
{
    // store the number of bytes available from the serial port
    int bytesAvail = this->available();
    if (this->ready && bytesAvail > 0 && !this->hasInput)
    {
        if (this->hasData)
        {
            // need to clear if there is existing input and the user didn't process the data
            this->input.clear();
            this->hasData = false;
        }
        // add data until we have a header or run out of bytes
        for (int i = 0; i < bytesAvail; i++)
        {
            this->input.append(this->readC());
            if (this->input.size >= GSMessage::headerLen)
                break;
        }

        // if we have a header
        if (this->input.size >= GSMessage::headerLen)
        {
            // decode header
            this->input.decodeHeader();

            // start the input timeout
            this->inputTimer = this->time();

            // check if we got a valid headers
            if (this->input.dataType > 0 && this->input.id > 0 && this->input.msgSize > 0)
            {
                // we have a valid message
                this->hasInput = true;

                // update available bytes
                bytesAvail = this->available();

                // fill message with available data
                for (int i = 0; i < bytesAvail; i++)
                {
                    if (this->input.size == this->input.msgSize + GSMessage::headerLen)
                        break;
                    this->input.append(this->readC());
                }
            }
            else
            {
                // flush input buffer
                while (this->available())
                    this->readC();

                if (this->handshake)
                {
                    this->logM(LL_ERROR, "Failed to parse header, flushing input buffer");
                }
            }
        }
    }

    // if we have a valid header
    if (this->hasInput)
    {
        // update available bytes
        bytesAvail = this->available();

        // fill message with available data until we have the full message
        for (int i = 0; i < bytesAvail; i++)
        {
            this->input.append(this->readC());
            if (this->input.size == this->input.msgSize + GSMessage::headerLen)
                break;
        }

        // reset timeout if data available
        if (bytesAvail > 0)
            this->inputTimer = this->time();

        // if we have the full message
        if (this->input.size == this->input.msgSize + GSMessage::headerLen)
        {
            // handle GSControl as a special case, since these are directed at this device and will not be sent via radio
            if (this->input.dataType == GSControl::type)
            {
                // decode the message
                GSControl cont;
                this->input.decode(&cont);

                // get the command
                char *cmd = nullptr;
                uint16_t argc = 0;
                char **argv = nullptr;
                cont.retrieveCmd(&cmd, &argc, &argv);
                // pass through default command handler
                if (cmd != nullptr && ((argc > 0 && argv != nullptr) || (argc == 0)) && !this->defaultControlHandler(cmd, argc, argv) && this->handshake)
                {
                    // if that doesn't work, pass through user handler
                    if (this->userControlHandler == nullptr || !this->userControlHandler(cmd, argc, argv))
                        // and if that doesn't work make it available to the user to handle manually
                        this->hasData = true;
                    else // processed with user handler, so clear the message
                        this->input.clear();
                }
                else // processed with default handler, so clear the message
                    this->input.clear();

                // clean up memory
                cont.cleanup(argc, &argv);
            }
            else if (this->handshake)
            {
                // there is no default handler for this, so pass through user handler
                if (this->userDataHandler == nullptr || !this->userDataHandler(&(this->input)))
                    // and if that doesn't work make it available to the user to handle manually
                    this->hasData = true;
                else // processed with user handler, so clear the message
                    this->input.clear();
            }
            else // if there is no handshake then no data that is not GSControl is valid, so clear the message
                this->input.clear();

            this->hasInput = false;
        }
        else if (this->input.size > this->input.msgSize + GSMessage::headerLen)
        {
            // something is wrong, there's more bytes in the message than the message length, so reset
            this->hasInput = false;
            this->input.clear();
        }

        if (this->time() - this->inputTimer > 50) // 50 ms timeout
        {
            // likely there is an issue with decoding the header
            // BUT, note that this could cause issues later
            this->hasInput = false;
            this->input.clear();
        }
    }
    // send Metrics on timer and check if handshake was successful
    if (this->time() - this->metricsTimer > this->metricsInterval && this->handshake)
    {
        // reset timer
        this->metricsTimer = this->time();
        // loop through metrics to send each
        for (int i = 0; i < this->numMetrics; i++)
        {
            // clear message
            this->output.clear();
            // set correct metadata
            this->output.setMetadata(Metrics::type, this->statusId);
            // encode metrics
            this->output.encode(this->metricsArr[i]);
            // write multiplexed data
            this->write((char *)this->output.buf, this->output.size);
        }
    }

    // return whether there is data to be handled manually
    return this->hasData;
}

bool GSInterface::isReady()
{
    // check whether the interface is ready, a handshake is established, the interface is not busy
    return this->ready && this->handshake && !(this->state == IS_SLEEP || this->state == IS_HANDSHAKE);
}

GSStream GSInterface::createStream(uint8_t type, uint8_t deviceId)
{
    // check for a metrics with this device id
    bool hasDeviceId = false;
    uint16_t i;
    for (i = 0; i < this->numMetrics; i++)
    {
        if (this->metricsArr[i]->deviceId == deviceId)
        {
            hasDeviceId = true;
            break;
        }
    }

    // if there is no metrics with this device id, and numMetrics is not max(uint16), create a new one
    if (!hasDeviceId && this->numMetrics < 0xFFFF)
    {
        // create new, larger metrics array
        Metrics **newMetricsArr = new Metrics *[this->numMetrics + 1];
        // copy pointers from old array
        memcpy(newMetricsArr, this->metricsArr, sizeof(Metrics *) * this->numMetrics);
        // add new pointer to array
        newMetricsArr[this->numMetrics] = new Metrics(deviceId);
        // delete old array
        delete[] this->metricsArr;
        // set metricsArr to new array
        this->metricsArr = newMetricsArr;
        // create a new stream with the given type, the next stream index, and the new metrics
        return GSStream(type, streamIndex++, this->metricsArr[numMetrics++]);
    }

    // create a new stream with the given type, the next stream index, and the found metrics
    return GSStream(type, streamIndex++, this->metricsArr[i]);
}

int GSInterface::writeStream(GSStream *s, Data *data, short signalStrength)
{
    // clear existing data
    this->output.clear();
    // set proper type and id
    this->output.setMetadata(s->type, s->id);
    // encode data to message
    this->output.encode(data);
    // update metrics
    s->streamMetrics->update(this->output.size, this->time(), signalStrength);
    // write the stream data
    return this->write((char *)this->output.buf, this->output.size);
}

int GSInterface::writeStream(GSStream *s, char *data, int dataLen, short signalStrength)
{
    // make sure there is data to send
    if (dataLen <= 0)
        return 0;

    // clear existing data
    this->output.clear();
    // set proper type and id
    this->output.setMetadata(s->type, s->id);
    // set up data to be encoded for multiplexing
    this->output.encode((uint8_t *)data, dataLen);
    // update metrics for this stream
    s->streamMetrics->update(this->output.size, this->time(), signalStrength);
    // write the stream data
    return this->write((char *)this->output.buf, this->output.size);
}

bool GSInterface::readInput(Data *data)
{
    // check if there is input available
    if (this->hasData)
    {
        this->input.decode(data);

        this->input.clear();

        this->hasData = false;
        return true;
    }
    return false;
}

// setters for user defined handlers
void GSInterface::setUserControlHandler(GSControl_CB c) { this->userControlHandler = c; }
void GSInterface::setUserDataHandler(GSInterface_DataCB c) { this->userDataHandler = c; }
void GSInterface::setUserModeHandler(GSInterface_ModeCB c) { this->userModeHandler = c; }

// clearers for user defined handlers
void GSInterface::clearUserControlHandler() { this->userControlHandler = nullptr; }
void GSInterface::clearUserDataHandler() { this->userDataHandler = nullptr; }
void GSInterface::clearUserModeHandler() { this->userModeHandler = nullptr; }

void GSInterface::logM(LogLevel lvl, const char *str)
{
    // make sure there is a serial port
    if (this->s != nullptr && this->ready)
    {
        GSControl cont;
        // add log level to the front
        char s[GSControl::maxArgSize];
        snprintf(s, GSControl::maxArgSize, "%s %s", logLevelStr[lvl], str);
        // get the correct GSControl command
        switch (lvl)
        {
        case LL_FATAL:
            cont.setCmd("FATAL", s);
            break;

        default:
            cont.setCmd("LOG", s);
            break;
        }

        this->output.setMetadata(GSControl::type, this->statusId);
        // encode message
        this->output.encode(&cont);
        // write multiplexed data
        this->write((char *)this->output.buf, this->output.size);
    }
}

void GSInterface::log(const char *str1, const char *str2, const char *str3)
{
    // make sure there is a debug serial port
    if (this->sd != nullptr && this->debugBaud > 0)
    {
        // platform dependent implementation
#ifdef ARDUINO
        this->sd->print(str1);
        this->sd->print(str2);
        this->sd->print(str3);
        this->sd->write("\n");
#endif
    }
}

// private methods

bool GSInterface::defaultControlHandler(char *cmd, uint16_t argc, char **argv)
{
    // returns true if the command is found, with no reference to whether it was processed correctly
    // this allows us to not call the user handler needlessly, and only if the command is not one of the
    // default commands
    if (strcmp(cmd, "RESET") == 0)
        this->reset(); // don't bother returning true here, since the code will restart anyway
    else if (strcmp(cmd, "HANDSHAKE") == 0)
    {
        // begin the handshake
        this->handshake = false;
        this->state = IS_HANDSHAKE;

        // if not 1 arg, there's an issue with the message
        if (argc == 1)
        {
            // repeat back code and end with newline
            this->write(argv[0], strlen(argv[0]));
            this->writeC('\n');
        }
        else
            this->state = IS_NORMAL;

        // successfully found command
        return true;
    }
    // all other commands are invalid if no handshake
    if (this->handshake || this->state == IS_HANDSHAKE)
    {
        // HS_DONE only valid if there is an active handshake
        if (this->state == IS_HANDSHAKE && strcmp(cmd, "HS_DONE") == 0)
        {
            // if not 1 arg, there's an issue with the message
            if (argc == 1)
            {
                if (strcmp(argv[0], "SUCCESS") == 0)
                {
                    // the handshake was successful
                    this->handshake = true;
                    this->state = IS_NORMAL;
                    for (int i = 0; i < this->numMetrics; i++)
                    {
                        this->metricsArr[i]->setInitialTime(this->time());
                    }

                    char str[sizeof("Interface ") + sizeof(GSInterface::version)];
                    snprintf(str, sizeof(str), "Interface %s", GSInterface::version);
                    this->logM(LL_DEBUG, str);
                }
                else if (strcmp(argv[0], "FAIL") == 0)
                {
                    // the handshake was not successful
                    this->handshake = false;
                    this->state = IS_NORMAL;
                }
            }
            else
            {
                // something is wrong with the handshake, so reset
                this->handshake = false;
                this->state = IS_NORMAL;
            }

            // successfully found command
            return true;
        }
        else if (strcmp(cmd, "SET_MODE") == 0)
        {
            // if not 1 arg, there's an issue with the message
            if (argc == 1)
            {
                // find the correct state
                bool newState = false;
                if (strcmp(argv[0], "SLEEP") == 0)
                {
                    // check if this is actually a state switch, or just setting to the current state
                    newState = IS_SLEEP != this->state;
                    this->state = IS_SLEEP;
                }
                else if (strcmp(argv[0], "NORMAL") == 0)
                {
                    newState = IS_NORMAL != this->state;
                    this->state = IS_NORMAL;
                }
                else if (strcmp(argv[0], "HITL") == 0)
                {
                    newState = IS_HITL != this->state;
                    this->state = IS_HITL;
                }

                if (newState)
                {
                    char s[sizeof("Changing mode to: ") + 10];
                    snprintf(s, sizeof(s), "Changing mode to: %s", argv[0]);
                    this->logM(LL_DEBUG, s);
                }

                if (newState && this->userModeHandler != nullptr)
                    this->userModeHandler(this->state);
            }
            // successfully found command
            return true;
        }
    }
    return false;
}

int GSInterface::available()
{
    // platform dependent implementation
#ifdef ARDUINO
    return this->s->available();
#else
    return 0;
#endif
}

int GSInterface::write(char *s, uint32_t length)
{
    // platform dependent implementation
#ifdef ARDUINO
    return this->s->write(s, length);
#endif
}

void GSInterface::writeC(char c)
{
    // platform dependent implementation
#ifdef ARDUINO
    this->s->write(c);
#endif
}

int GSInterface::read(char *s, uint32_t length)
{
    // platform dependent implementation
#ifdef ARDUINO
    uint32_t addedLength = 0;
    // add bytes while there are still bytes in the serial port and the array is not full
    while (this->s->available() > 0 && addedLength < length)
    {
        s[addedLength++] = this->s->read();
    }
    return addedLength;
#else
    return 0;
#endif
}

char GSInterface::readC()
{
    // platform dependent implementation
#ifdef ARDUINO
    return this->s->read();
#else
    return 0;
#endif
}

uint32_t GSInterface::time()
{
    // platform dependent implementation
#ifdef ARDUINO
    return millis();
#else
    return 0;
#endif
}

void GSInterface::reset()
{
    // platform dependent implementation
#ifdef ARDUINO
    // should be able to cause a crash + reboot by purposefully dereferencing a null pointer
    char *p = {0};
    *p = 0;
#else
#endif
}
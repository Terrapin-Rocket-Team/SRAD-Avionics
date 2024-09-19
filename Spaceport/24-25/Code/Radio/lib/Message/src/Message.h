#ifndef MESSAGE_H
#define MESSAGE_H

#if defined(ARDUINO)
#include <Arduino.h>
#elif defined(_WIN32) || defined(_WIN64) || defined(__unix__) || defined(__APPLE__) // Windows, Linux, or OSX
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cmath>
using namespace std;
#endif

#include "Data.h"

class Message
{
public:
    // maximum message size
    static const uint16_t maxSize = 10e3;

    // the buffer that stores the message data
    // acutal size is maxSize+1, but the last byte should always be 0 to prevent issues with C string functions
    uint8_t buf[maxSize + 1] = {0};
    // size of the stored message, max is maxSize
    uint16_t size = 0;
    // separation character for multiple messages
    char sep = 0;

    // Message default constructor
    // - sep : the separation character when combining multiple messages
    Message(char sep = 0) : sep(sep) {};

    // Message constructor
    // - rawData : an encoded message
    // - sz : the size of the encoded message
    // - sep : the separation character when combining multiple messages
    Message(uint8_t rawData[maxSize], uint16_t sz, char sep = 0);

    // Message constructor
    // - data : a ```Data``` object that will be encoded into the Message buffer
    // - sep : the separation charaacter when combining multiple messages
    Message(Data *data, char sep = 0);

    // use return type Message* so we can stack operators e.g., ```Message()->fill()->encode()```

    // encodes ```data``` and places the output in the Message buffer
    // Note: cannot combine multiple messages using VideoData
    Message *encode(Data *data, bool append = false);
    // decodes ```data``` using the Message buffer, ```data``` is populated with the decoded information
    Message *decode(Data *data);
    // clears all stored data
    Message *clear();

    // append the contents of ```data``` to the Message buffer, where ```data``` contains ```sz``` bytes, fails if final message size will be too large
    // Note: does not add message separator, use ```encode()``` to combine multiple messages
    Message *append(uint8_t *data, uint16_t sz);
    // remove the last ```sz``` bytes from the Message buffer and place them in ```data```, ```sz``` is set to the number of bytes copied
    Message *pop(uint8_t *data, uint16_t &sz);
    // remove the first ```sz``` bytes from the Message buffer and place them in Packet ```data```, ```sz``` is set to the number of bytes copied
    Message *shift(uint8_t *data, uint16_t &sz);

    // utility methods

    // copy ```sz``` bytes from ```data``` into the Message buffer, ```sz``` must be less than ```Message.maxSize```
    Message *fill(uint8_t *data, uint16_t sz);
    // copy ```sz``` bytes from ```data``` into the Message buffer starting at index ```start```, ```sz``` must be less than ```Message.maxSize - start```
    Message *fill(uint8_t *data, uint16_t start, uint16_t sz);

    // copies ```this->size``` bytes from the Message buffer into ```data```
    Message *get(uint8_t *data);
    // copies ```sz``` bytes from the Message buffer into ```data```, sets ```sz``` to the number of bytes copied
    Message *get(uint8_t *data, uint16_t &sz);
    // copies ```sz``` bytes from the Message buffer into ```data``` starting from index ```start```, sets ```sz``` to the number of bytes copied
    Message *get(uint8_t *data, uint16_t &sz, uint16_t start);
    // copies from the Message buffer into ```data``` starting from index ```start``` until index ```end```, sets ```sz``` to the number of bytes copied
    Message *get(uint8_t *data, uint16_t &sz, uint16_t start, uint16_t end);

#ifdef ARDUINO
    // prints the contents of the message over ```Serial```
    // Note: video not supported
    Message *print(Stream &Serial);

    // writes the contents of the message over ```Serial```
    // Note: video supported (though I don't know why you want to write raw video data to the console)
    Message *write(Stream &Serial);
#endif
};

#endif
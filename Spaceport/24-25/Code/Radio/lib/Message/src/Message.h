#ifndef MESSAGE_H
#define MESSAGE_H

#include "Arduino.h"
#include "Data.h"

class Message
{
public:
    // static vars
    static const uint16_t maxSize = 10e3;

    // instance vars
    uint8_t buf[maxSize + 1] = {0}; // add one extra byte that should always be 0 to prevent issues with C string functions
    uint16_t size = 0;
    char sep = 0;

    // methods
    // Message constructor
    // sep : the separation character when combining multiple messages
    Message(char sep = 0) : sep(sep) {};
    Message(uint8_t rawData[maxSize], uint16_t sz, char sep = 0);
    Message(Data *data, char sep = 0);

    // use return type Message* so we can stack operators e.g., ```Message()->fill()->encode()```

    // encodes ```data``` and places the output in the Message buffer
    Message *encode(Data *data);
    // decodes ```data``` using the Message buffer, ```data``` is populated with the decoded information
    Message *decode(Data *data);
    // clears all stored data
    Message *clear();

    // append the contents of ```data``` to the Message buffer, where ```data``` contains ```sz``` bytes, fails if final message size will be too large
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
};

#endif
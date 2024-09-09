#ifndef MESSAGE_H
#define MESSAGE_H

#include "Arduino.h"
#include "Data.h"
#include "Packet.h"

/*
Types:
- MSG_TELEMETRY: use for telemetry data
- MSG_VIDEO: use for video data
- MSG_COMMAND: use for commands
- MSG_NONE: use for empty message
*/
enum MessageType
{
    MSG_TELEMETRY,
    MSG_VIDEO,
    MSG_COMMAND,
    MSG_NONE
};

class Message
{
public:
    // static vars
    static const uint16_t maxSize = 10e3;

    // instance vars
    MessageType type = MSG_NONE;
    uint8_t buf[maxSize] = {0};
    uint16_t size = 0;

    // methods
    Message(MessageType type) : type() {};
    Message(MessageType type, uint8_t rawData[maxSize], uint16_t sz);
    Message(MessageType type, Data *data);

    // use return type Message* so we can stack operators e.g., ```Message()->fill()->encode()```

    // encodes ```data``` and places the output in the Message buffer
    Message *encode(Data *data);
    // decodes ```data``` using the Message buffer, ```data``` is populated with the decoded information
    Message *decode(Data *data);
    // clears all stored data
    Message *clear();

    // append the contents of ```p``` to the Message buffer
    Message *addPacket(Packet *p);
    // remove the last ```p->maxSize``` bytes from the Message buffer and place them in Packet ```p```
    Message *popPacket(Packet *p);
    // remove the first ```p->maxSize``` bytes from the Message buffer and place them in Packet ```p```
    Message *shiftPacket(Packet *p);
    // get ```p->maxSize``` bytes from the Message buffer and place them in Packet ```p``` for packet number ```index```
    Message *getPacket(Packet *p, uint16_t index);

    // utility methods

    // copy ```sz``` bytes from ```data``` into the Message buffer, ```sz``` must be less than ```Message.maxSize```
    Message *fill(uint8_t *data, uint16_t sz);
    // copy from ```data```, starting at index ```start``` until index ```end```, into the Message buffer, total bytes copied must be less than ```Message.maxSize```
    Message *fill(uint8_t *data, uint16_t start, uint16_t end);

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
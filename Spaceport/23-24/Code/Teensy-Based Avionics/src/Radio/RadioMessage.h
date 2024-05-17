#ifndef RADIO_MESSAGE_H
#define RADIO_MESSAGE_H

#include <cstdint>

const int RADIO_MESSAGE_BUFFER_SIZE = 255; // maximum length of an encoded `RadioMessage` in bytes.

class RadioMessage
{
public:
    RadioMessage();
    virtual bool encode() = 0;                           // use stored variables to encode a message
    virtual bool decode() = 0;                           // use `message` to set stored variables
    virtual int length() const;                          // return the length of the message
    virtual bool setArr(const uint8_t *srcArr, int len); // set the message to the given array
    virtual uint8_t *getArr();                           // get the message as an array
    virtual ~RadioMessage(){};

protected:
    uint8_t string[RADIO_MESSAGE_BUFFER_SIZE]; // buffer for the encoded message
    uint8_t len;                               // length of the encoded message
};

#endif // RADIO_MESSAGE_H
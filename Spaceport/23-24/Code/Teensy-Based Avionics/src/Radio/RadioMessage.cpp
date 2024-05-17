#include "RadioMessage.h"


RadioMessage::RadioMessage()
{
    this->len = 0;
}

/*
Copies a source array to the encoded message buffer
    \param srcArr: the array to copy
    \param len: the length of the source array
    \return `false` if the length of the source array is greater than the buffer size, `true` otherwise
*/
bool RadioMessage::setArr(const uint8_t *srcArr, int len)
{
    if (len > RADIO_MESSAGE_BUFFER_SIZE)
        return false;

    memcpy(this->string, srcArr, len);
    this->len = len;
    return true;
}

/*
Copies the encoded message buffer to a destination array
    \param destArr: the array to copy into
    \param len: the length of the dest array
    \return `false` if the length of the dest array is smaller than the buffer size, `true` otherwise
*/
uint8_t *RadioMessage::getArr()
{
    return string;
}
/*
Returns the length of the encoded message
*/
int RadioMessage::length() const
{
    return len;
}
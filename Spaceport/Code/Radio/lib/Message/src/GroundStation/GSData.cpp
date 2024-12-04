#include "GSData.h"

GSData::GSData(uint8_t streamIndex) : index(streamIndex) {}

GSData::GSData(uint8_t streamIndex, uint8_t *buf, uint16_t size) : index(streamIndex)
{
    this->fill(buf, size);
}

bool GSData::decodeHeader(uint16_t header, uint8_t &streamIndex, uint16_t &size)
{
    // header will be 0xLNMP so >> 12 to get 0x0L
    streamIndex = header >> 12;
    // then 0xLNMP & 0x0FFF to get 0x0NMP
    size = header & 0x0FFF;
    return true;
}

uint16_t GSData::encode(uint8_t *data, uint16_t size)
{
    uint16_t pos = 0;
    if (size < this->size)
        return 0; // error not enough space for message

    // header
    // index will be 0x0L so << 4 to make it 0xL0
    // size will be 0x0NMP so >> 8 to get 0x000N
    // then add to get 0xLN
    data[pos++] = (this->index << 4) + (this->size >> 8);
    // size will be 0x0NMP so & 0x00FF to get 0x00MP
    data[pos++] = this->size & 0x00FF;

    // body
    memcpy(data + pos, this->buf, this->size);

    return pos + this->size;
}

uint16_t GSData::decode(uint8_t *data, uint16_t size)
{
    uint16_t pos = 0;
    if (size > this->maxSize)
        return 0; // error data too big

    // header
    // first byte will be 0xLN so >> 4 to get 0x0L
    this->index = data[pos] >> 4; // don't shift because the other half of the byte is size info
    // still first byte, will be 0xLN so & 0x0F to get 0x0N, then << 8 to get 0x0N00
    this->size = (data[pos++] & 0x0F) << 8;
    // next byte is 0xMP, which is exactly what we need, so just add to get 0x0NMP
    this->size += data[pos++];

    // body
    memcpy(this->buf, data + pos, size - pos);
    return size;
}

GSData *GSData::fill(uint8_t *buf, uint16_t size)
{
    // check if size is too big
    if (size <= this->maxDataSize)
    {
        // if it is fine fill the buffer
        this->size = size;
        memcpy(this->buf, buf, size);
    }
    else
    {
        // otherwise only copy dataMaxSize bytes
        this->size = this->maxDataSize;
        memcpy(this->buf, buf, this->maxDataSize);
    }
    return this;
}

uint16_t GSData::toJSON(char *json, uint16_t sz, const char *streamName)
{
    uint16_t result = (uint16_t)snprintf(json, sz, "{\"type\": \"GSData\", \"name\":\"%s\", \"data\": {\"index\": %d, \"buf\": [", streamName, this->index);
    if (result >= sz)
    {
        // output too large
        return 0;
    }

    // result should be the index of the \0

    // need to dynamically adjust the number of bytes added based on size
    for (int i = 0; i < this->size; i++)
    {
        if (result + 4 < sz)
        {
            int added = sprintf(json + result, "%d,", this->buf[i]);
            if (added > 0 && result + added < sz)
                result += added;
            else
                return 0; // output too large
        }
        else
            return 0; // output too large
    }

    // result should be the index of \0
    // unless size is 0
    if (this->size == 0)
        result++;

    // add closing braces
    // need to overwrite the last trailing comma
    if (result + 3 < sz)
    {
        json[result - 1] = ']';
        json[result] = '}';
        json[result + 1] = '}';
        json[result + 2] = '\0';

        // ran properly
        return result;
    }

    // output too large
    return 0;
}

uint16_t GSData::fromJSON(char *json, uint16_t sz, char *streamName)
{
    char indexTxt[10] = {0};

    if (!extractStr(json, sz, "\"name\":\"", '"', streamName))
        return 0;
    if (!extractStr(json, sz, "\"index\": ", ',', indexTxt, 3))
        return 0;

    this->index = atoi(indexTxt);

    char *dataStrPos = strstr(json, "\"buf\": [");
    int current = int(dataStrPos - json) + 8; // add 8 to move to the "["
    this->size = 0;

    while (json[current] != ']' && current < sz)
    {
        char dataByte[4] = {0};
        int index = 0;
        while (json[current] != ',' && current < sz)
        {
            if (index < 3)
            {
                dataByte[index] = json[current];
                index++;
            }
            current++;
        }
        this->buf[this->size] = atoi(dataByte);
        this->size++;
        current++;
    }
    return this->size;
}
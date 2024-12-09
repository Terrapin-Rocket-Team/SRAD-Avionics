#ifndef DATA_H
#define DATA_H

#if defined(ARDUINO)
#include <Arduino.h>
#elif defined(_WIN32) || defined(_WIN64) || defined(__unix__) || defined(__APPLE__) // Windows, Linux, or OSX
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cmath>
using namespace std;
#endif

// basically just a container to place decoded data into
class Data
{
public:
    virtual ~Data() {}; // Virtual descructor. Very important
    // encode the data stored in the ```Data``` object and place the result in ```data```, ```sz``` is the max size of ```data```
    virtual uint16_t encode(uint8_t *data, uint16_t sz) = 0;
    // decode the data stored in ```data``` and place it in the ```Data``` object, ```sz``` is the number of bytes from ```data``` to decode
    virtual uint16_t decode(uint8_t *data, uint16_t sz) = 0;

    virtual uint16_t toJSON(char *json, uint16_t sz, const char *streamName = "") = 0;
    virtual uint16_t fromJSON(char *json, uint16_t sz, char *streamName) = 0;

    bool extractStr(char *src, int szSource, const char *lookFor, char stopCond, char *dest, int szDest = -1)
    {
        char *strPos = strstr(src, lookFor);
        int pos = int(strPos - src) + strlen(lookFor);
        int counter = 0;
        while (src[pos] != stopCond && pos < szSource && (szDest == -1 || counter < szDest - 1))
        {
            dest[counter++] = src[pos++];
        }
        return true;
    }
};

#endif
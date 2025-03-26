#ifndef CRCGEN_H
#define CRCGEN_H

#include "definitions.h"

class CRC
{
public:
    BIT16 crc_ccitt(unsigned char *msg, int len);
    BIT16 crchware(BIT16 data, BIT16 genpoly, BIT16 accum);
};

#endif
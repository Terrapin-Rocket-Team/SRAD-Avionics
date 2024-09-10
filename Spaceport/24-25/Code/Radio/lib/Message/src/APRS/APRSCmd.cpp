#include "APRSCmd.h"

APRSCmd::APRSCmd(APRSConfig config) : APRSData(config)
{
}
APRSCmd::APRSCmd(APRSConfig config, uint8_t cmd, uint16_t args) : APRSData(config), cmd(cmd), args(args)
{
}

uint16_t APRSCmd::encode(uint8_t *data, uint16_t sz)
{
    uint16_t pos = 0;

    // APRS header
    this->encodeHeader(data, sz, pos);

    // command ("ccaaa")
    // cc = command
    // aaa = arguments

    if (sz < pos + 5)
        return 0; // error too small for command

    base10toBase91(data, pos, this->cmd, 2);  // this is quite inefficient (2^8 = 256, but we are using 91^2 = 8281 to represent it)
    base10toBase91(data, pos, this->args, 3); // also quite inefficient (2^16 = 65536, but we are using 91^3 = 753571 to represent it)

    return pos;
}

uint16_t APRSCmd::decode(uint8_t *data, uint16_t sz)
{
    uint16_t pos = 0;
    uint32_t decodedNum = 0;

    this->decodeHeader(data, sz, pos);

    // command ("ccaaa")
    // cc = command
    // aaa = arguments

    if (sz < pos + 5)
        return 0; // error too small for command

    base91toBase10(data, pos, decodedNum, 2);
    this->cmd = decodedNum & 0x00FF;
    base91toBase10(data, pos, decodedNum, 3);
    this->args = decodedNum;

    return pos;
}
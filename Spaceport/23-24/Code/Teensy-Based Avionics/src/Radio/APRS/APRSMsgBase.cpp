#include "APRSMsgBase.h"


APRSMsgBase::APRSMsgBase(APRSHeader header)
{
    this->header = header;
}

bool APRSMsgBase::encode()
{
    int c = encodeHeader();
    encodeData(c);
    return true;
}

bool APRSMsgBase::decode()
{
    // message needs to be decoded and values reinserted into the header and data structs
    int c = decodeHeader();
    decodeData(c);
    return true;
}

#pragma region APRSMsgBase Header Encoding/Decoding

int APRSMsgBase::encodeHeader()
{
    // format: CALLSIGN>TOCALL,PATH:
    int cursor = 0;

    for (int i = 0; header.CALLSIGN[i] && i < 8; i++, cursor++)
        string[cursor] = header.CALLSIGN[i];

    string[cursor++] = '>';

    for (int i = 0; header.TOCALL[i] && i < 8; i++, cursor++)
        string[cursor] = header.TOCALL[i];

    string[cursor++] = ',';

    for (int i = 0; header.PATH[i] && i < 10; i++, cursor++)
        string[cursor] = header.PATH[i];

    string[cursor++] = ':'; // end of header
    return cursor;
}

int APRSMsgBase::decodeHeader()
{
    // format: CALLSIGN>TOCALL,PATH:
    int cursor = 0;

    for (int i = 0; string[cursor] != '>'; i++, cursor++)
        header.CALLSIGN[i] = string[cursor];
    header.CALLSIGN[cursor] = '\0';

    cursor++; // skip '>'

    for (int i = 0; string[cursor] != ','; i++, cursor++)
        header.TOCALL[i] = string[cursor];
    header.TOCALL[cursor] = '\0';

    cursor++; // skip ','

    for (int i = 0; string[cursor] != ':'; i++, cursor++)
        header.PATH[i] = string[cursor];
    header.PATH[cursor] = '\0';

    cursor++; // skip ':'
    return cursor;
}

#pragma endregion

#pragma region Base91 Encoding

void APRSMsgBase::encodeBase91(uint8_t *message, int &cursor, int value, int precision)
{
    for (int i = precision - 1; i >= 0; i--)
    {
        int divisor = pow(91, i);
        message[cursor++] = (uint8_t)((int)(value / divisor) + 33);
        value %= divisor;
    }
}

void APRSMsgBase::decodeBase91(const uint8_t *message, int &cursor, double &value, int precision)
{
    value = 0;
    for (int i = 0; i < precision; i++)
    {
        value += (message[cursor++] - 33) * pow(91, precision - i - 1);
    }
}

#pragma endregion
#ifndef BMP390_H
#define BMP390_H

#include <Adafruit_BMP3XX.h>
#include "Barometer.h"

namespace mmfs
{
    class BMP390 : public Barometer
    {
    private:
        Adafruit_BMP3XX bmp;

    public:
        BMP390(const char *name = "BMP390");
        virtual bool init() override;
        virtual void read() override;
    };
}

#endif // BMP390_H

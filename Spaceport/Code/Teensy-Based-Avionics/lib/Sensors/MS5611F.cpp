
#include "MS5611F.h"

namespace mmfs
{
    mmfs::MS5611::MS5611(const char *name) : ms(0x76)
    {
        setName(name);
    }

    bool mmfs::MS5611::init()
    {
        if (!ms.begin())
        {
            printf("Failed to initialize MS5611 sensor\n");
            return initialized = false;
        }
        ms.setOversampling(OSR_ULTRA_LOW);

        return initialized = true;
    }
    void mmfs::MS5611::read()
    {
        ms.read();
        temp = ms.getTemperature();
        pressure = ms.getPressure();
    }

}

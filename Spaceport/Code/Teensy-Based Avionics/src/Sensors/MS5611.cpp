
#include "MS5611.h"


namespace mmfs
{
   MS5611::MS5611(const char *name)
   {
       setName(name);
   }


   bool MS5611::init()
   {
       if(!ms.begin())
       {
           return false;
       }
       ms.setOversampling(OSR_ULTRA_LOW) 

       return true;
   }
   void MS5611::read()
   {
       temperature = ms.getTemperature();
       pressure = ms.getPressure();
   }

}

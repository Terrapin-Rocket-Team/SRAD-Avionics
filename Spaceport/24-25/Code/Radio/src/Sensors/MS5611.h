#include "MS5611.h"


namespace mmfs
{
   MS5611::MS5611(const char *name)
   {
       setName(name);
   }


   bool MS5611::init()
   {
       if(!begin())
       {
           return false;
       }
       setOversampling();


       return true;
   }
   void MS5611::read()
   {
       temperature = getTemperature();
       pressure = getPressure();
   }
}

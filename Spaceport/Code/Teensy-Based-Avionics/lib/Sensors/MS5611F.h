#ifndef MS5611F_H
#define MS5611F_H


#include <MS5611.h>
#include "Sensors/Baro/Barometer.h"


namespace mmfs
{
   class MS5611 : public Barometer
   {
       private:
           ::MS5611 ms;
  
   public:
       MS5611(const char *name = "MS5611");
       virtual bool init() override;
       virtual void read() override;
   };


}
#endif

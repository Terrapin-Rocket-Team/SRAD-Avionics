#ifndef MS5611_H
#define MS5611_H


#include <MS5611.h>
#include "Sensors/Baro/Barometer.h"


namespace mmfs
{
   class MS5611 : public Barometer
   {
       private:
           MS5611 ms;
  
   public:
       MS5611("MS5611");
       virtual bool init() override;
       virtual void read() override;
   };


}
#endif

#include "AviEventListener.h"
#include <Events/DefaultEvents.h>
void AviEventLister::onEvent(const Event *e)
{
    if (e->ID == "GPS_FIX"_i)
    {
        GPSFix *gfe = (GPSFix *)e;
        if(gfe->hasFix){
            bb.clearQueue(32);
            bb.on(32);
        }
        else{
            bb.clearQueue(32);
            bb.aonoff(32, patt, true);
        }

    }
}
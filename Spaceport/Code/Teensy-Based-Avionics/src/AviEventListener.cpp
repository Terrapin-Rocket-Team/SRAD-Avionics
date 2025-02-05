#include "AviEventListener.h"
#include <Events/DefaultEvents.h>
void AviEventLister::onEvent(const Event *e)
{
    if (e->ID == "GPS_FIX"_i)
    {
        GPSFix *gfe = (GPSFix *)e;
        if(gfe->hasFix){
            bb.clearQueue(17);
            bb.on(17);
        }
        else{
            bb.clearQueue(17);
            bb.aonoff(17, patt, true);
        }

    }
}
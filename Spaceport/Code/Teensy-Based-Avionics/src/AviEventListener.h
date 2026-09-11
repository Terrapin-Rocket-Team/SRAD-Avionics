#ifndef AVIEVENTLISTENER_H
#define AVIEVENTLISTENER_H

#include <Events/Event.h>
#include <BlinkBuzz/BlinkBuzz.h>
using namespace mmfs;
class AviEventLister : public IEventListener {

public:
    AviEventLister() : IEventListener() {};
    void onEvent(const Event *e) override;
    BBPattern patt = BBPattern(200, 1);
};

#endif // AVIEVENTLISTENER_H
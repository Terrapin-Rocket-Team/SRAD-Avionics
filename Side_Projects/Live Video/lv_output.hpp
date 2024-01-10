#ifndef LV_OUTPUT_H
#define LV_OUTPUT_H

#include "output/output.hpp"

#include <unistd.h>

class LVOutput : Output
{
public:
    LVOutput(VideoOptions const *options);
    ~LVOutput();

protected:
    void outputBuffer(void *mem, size_t size, int64_t timestamp_us, uint32_t flags) override;

private:
    int pidLocal;
    int pidLive;
    int pipeLocal[2];
    int pipeLive[2];
};

#endif
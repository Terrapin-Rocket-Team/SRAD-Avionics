#include "Encoder.h"

namespace mmfs
{

    const char *Encoder::getCsvHeader() const
    {
        return "E-Rel Steps";
    }

    const char *Encoder::getDataString() const
    {
        sprintf(data, "%d,", currentRelativeSteps);
        return data;
    }

    const char *Encoder::getStaticDataString() const
    {
        sprintf(staticData, "Initial Steps: %d\n", initialSteps);
        return staticData;
    }

    void Encoder::update()
    {
        read();
    }

    bool Encoder::begin(bool useBiasCorrection)
    {
        biasCorrectionMode = useBiasCorrection;
        return init();
    }
}
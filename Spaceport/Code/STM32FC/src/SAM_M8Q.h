#ifndef SAM_M8Q_H
#define SAM_M8Q_H

#include "Sensors/GPS/MAX_M10S.h"

namespace astra
{
    class SAM_M8Q : public MAX_M10S
    {
    public:
        using MAX_M10S::MAX_M10S; // inherit ctors
    };
}

#endif

#ifndef VSDK_RAY_CROSSING_SINK_H
#define VSDK_RAY_CROSSING_SINK_H

#include "vsdk/toolkit/environment/geometry/element/RayCrossing.h"

class RayCrossingSink {
public:
    virtual ~RayCrossingSink();
    virtual void crossing(const RayCrossing& event) = 0;
};

#endif

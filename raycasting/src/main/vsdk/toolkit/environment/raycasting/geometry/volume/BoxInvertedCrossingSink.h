#ifndef VSDK_BOX_INVERTED_CROSSING_SINK_H
#define VSDK_BOX_INVERTED_CROSSING_SINK_H

#include "vsdk/toolkit/environment/geometry/element/RayCrossingSink.h"

class BoxInvertedCrossingSink : public RayCrossingSink {
private:
    RayCrossingSink& destination;
public:
    explicit BoxInvertedCrossingSink(RayCrossingSink& destination);
    void crossing(const RayCrossing& event) override;
};
#endif

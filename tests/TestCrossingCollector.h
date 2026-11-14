#ifndef VSDK_TEST_CROSSING_COLLECTOR_H
#define VSDK_TEST_CROSSING_COLLECTOR_H

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingSink.h"

class TestCrossingCollector : public RayCrossingSink {
public:
    java::ArrayList<RayCrossing*> hits;
    ~TestCrossingCollector() override;
    void crossing(const RayCrossing& hit) override;
};
#endif

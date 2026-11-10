#ifndef VSDK_RAY_CROSSING_TEST_COLLECTOR_H
#define VSDK_RAY_CROSSING_TEST_COLLECTOR_H

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingSink.h"

class RayCrossingTestCollector : public RayCrossingSink {
public:
    java::ArrayList<double> roots;
    java::ArrayList<RayCrossingKind> kinds;
    java::ArrayList<const Geometry*> geometries;

    RayCrossingTestCollector();
    ~RayCrossingTestCollector() override;
    void crossing(const RayCrossing& event) override;
};

#endif

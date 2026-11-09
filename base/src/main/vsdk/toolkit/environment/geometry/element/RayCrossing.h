#ifndef VSDK_RAY_CROSSING_H
#define VSDK_RAY_CROSSING_H

#include "vsdk/toolkit/environment/geometry/element/RayCrossingKind.h"

class Geometry;
class SimpleBody;
class Material;

// Leaf identity and optional instance/material attribution; pointers are borrowed.
class RayCrossing {
public:
    using Kind = RayCrossingKind;
    double t;
    const Geometry* geometry;
    Kind kind;
    const SimpleBody* body;
    Material* material;

    RayCrossing(double t, const Geometry* geometry, Kind kind);
};

#endif

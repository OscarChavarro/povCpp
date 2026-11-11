#ifndef VSDK_RAY_CROSSING_H
#define VSDK_RAY_CROSSING_H

#include "vsdk/toolkit/environment/geometry/element/RayCrossingKind.h"
#include "vsdk/toolkit/environment/geometry/element/RayBoundary.h"

class Geometry;
class SimpleBody;
class Material;

// Leaf identity and optional instance/material attribution; pointers are borrowed.
class RayCrossing : public RayBoundary {
public:
    using Kind = RayCrossingKind;
    Kind kind;
    const SimpleBody* body;
    Material* material;

    RayCrossing(double t, const Geometry* geometry, Kind kind);
};

#endif

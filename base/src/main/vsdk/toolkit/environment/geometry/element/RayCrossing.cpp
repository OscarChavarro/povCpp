#include "vsdk/toolkit/environment/geometry/element/RayCrossing.h"

RayCrossing::RayCrossing(double t, const Geometry* geometry, Kind kind)
    : t(t), geometry(geometry), kind(kind), body(nullptr), material(nullptr)
{
}

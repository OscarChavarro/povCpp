#include "vsdk/toolkit/environment/geometry/element/RaySolidBoundary.h"

RaySolidBoundary::RaySolidBoundary(double t, const Geometry* geometry, bool normalReversed)
    : t(t), geometry(geometry), normalReversed(normalReversed) {}

RaySolidBoundary::RaySolidBoundary() : t(0), geometry(nullptr), normalReversed(false) {}

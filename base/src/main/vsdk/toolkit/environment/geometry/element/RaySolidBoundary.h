#ifndef VSDK_RAY_SOLID_BOUNDARY_H
#define VSDK_RAY_SOLID_BOUNDARY_H

class Geometry;

// Mathematical provenance only. Scene resolves appearance from the borrowed leaf.
class RaySolidBoundary {
public:
    double t;
    const Geometry* geometry;
    bool normalReversed;

    RaySolidBoundary();
    RaySolidBoundary(double t, const Geometry* geometry, bool normalReversed = false);
};

#endif

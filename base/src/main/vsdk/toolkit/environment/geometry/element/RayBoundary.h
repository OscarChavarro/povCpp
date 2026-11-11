#ifndef VSDK_RAY_BOUNDARY_H
#define VSDK_RAY_BOUNDARY_H

#include "vsdk/toolkit/environment/geometry/element/RayInstancePath.h"
class Geometry;

// Mathematical provenance only. Scene resolves appearance from the borrowed leaf.
class RayBoundary {
public:
    double t;
    const Geometry* geometry;
    bool normalReversed;
private:
    // Ordered from the leaf instance outward. Values are snapshots, identities borrowed.
    RayInstancePath path;
public:
    RayInstancePath& getProvenance();
    const RayInstancePath& getProvenance() const;
    bool appendInstance(const RayInstanceStep& step);
    long getInstanceCount() const;
    bool getInstance(long index, RayInstanceStep& result) const;
    void clearInstances();

    RayBoundary();
    RayBoundary(const RayBoundary& other);
    RayBoundary& operator=(const RayBoundary& other);
    ~RayBoundary();
    RayBoundary(double t, const Geometry* geometry, bool normalReversed = false);
};

#endif

#ifndef VSDK_RAYCASTING_SPHERE_TRACE_H
#define VSDK_RAYCASTING_SPHERE_TRACE_H

class GeometryStatistics;
class Sphere;
class Vector3Dd;

// POV tracing policy around VITRAL's mathematical root kernel.
class SphereTrace {
public:
    static bool intersect(const Sphere& sphere, const Vector3Dd& origin,
        const Vector3Dd& direction, GeometryStatistics* stats,
        double* depth1, double* depth2);
};

#endif

#include "vsdk/toolkit/common/statistics/GeometryStatistics.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "environment/geometry/element/GeometryConfig.h"
#include "environment/geometry/element/SphereTrace.h"

bool SphereTrace::intersect(const Sphere& sphere, const Vector3Dd& origin,
    const Vector3Dd& direction, GeometryStatistics* stats,
    double* depth1, double* depth2)
{
    stats->incrementRaySphereTests();
    const RayCrossingQuery query(GeometryConfig::SMALL_TOLERANCE,
        GeometryConfig::MAX_DISTANCE, GeometryConfig::SMALL_TOLERANCE,
        GeometryConfig::SMALL_TOLERANCE);
    if (Sphere::intersectionRoots(origin, direction, sphere.getRadiusReference(),
            query, *depth2, *depth1) == 0) return false;
    stats->incrementRaySphereTestsSucceeded();
    return true;
}

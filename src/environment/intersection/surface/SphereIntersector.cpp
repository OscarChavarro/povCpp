#include "vsdk/toolkit/common/statistics/GeometryStatistics.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "environment/intersection/ray/IntersectionTolerances.h"
#include "environment/intersection/surface/SphereIntersector.h"

bool SphereIntersector::intersect(const Sphere& sphere, const Vector3Dd& origin,
    const Vector3Dd& direction, GeometryStatistics* stats,
    double* depth1, double* depth2)
{
    stats->incrementRaySphereTests();
    const RayCrossingQuery query(IntersectionTolerances::SMALL_TOLERANCE,
        IntersectionTolerances::MAX_DISTANCE, IntersectionTolerances::SMALL_TOLERANCE,
        IntersectionTolerances::SMALL_TOLERANCE);
    if (Sphere::intersectionRoots(origin, direction, sphere.getRadiusReference(),
            query, *depth2, *depth1) == 0) return false;
    stats->incrementRaySphereTestsSucceeded();
    return true;
}

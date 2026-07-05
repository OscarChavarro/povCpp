#ifndef __BAKED_QUADRIC_INTERSECTOR__
#define __BAKED_QUADRIC_INTERSECTOR__

#include "environment/geometry/volume/Quadric.h"
#include "render/raySharedCache/RaySharedCache.h"

// Ray-quadric intersection resolution for baked CSG operands: solves the
// quadric polynomial along the ray (using the ray's cached position/
// direction aggregates when the operand shares the ray's own space) and
// reports the two roots clipped to the valid depth range.
class BakedQuadricIntersector {
public:
    static bool intersectBakedQuadric(
        const Quadric &shape,
        RayWithTracingState *ray,
        const Vector3Dd &origin,
        const Vector3Dd &direction,
        bool sharesRaySpace,
        RaySharedCache &cache,
        int viewpointSlot,
        double *depth1,
        double *depth2);

    // Like intersectBakedQuadric but also sets trueMiss=true when the
    // discriminant indicates the ray definitively misses the quadric (safe
    // to skip plane candidate checks). Intended for the hot path in
    // SingleCorePlaneCsgTrace::traceTransformedNestedSingleCorePlaneOperandAllCrossings,
    // where the three polynomial coefficients (A/B/C) of
    // intersectBakedQuadricWithCoeffs are not needed and would add register
    // pressure on every call.
    static bool intersectBakedQuadricWithTrueMiss(
        const Quadric &shape,
        RayWithTracingState *ray,
        const Vector3Dd &origin,
        const Vector3Dd &direction,
        bool sharesRaySpace,
        RaySharedCache &cache,
        int viewpointSlot,
        double *depth1,
        double *depth2,
        bool &trueMiss);

    // Like intersectBakedQuadric but also outputs the quadric polynomial
    // coefficients A/B/C (where Q(origin + t*direction) = A*t^2 + B*t + C)
    // and sets trueMiss=true when the discriminant is negative (ray
    // definitively misses the quadric - safe to skip plane checks). When
    // trueMiss=false and the function returns false, the ray may be inside
    // the quadric.
    static bool intersectBakedQuadricWithCoeffs(
        const Quadric &shape,
        RayWithTracingState *ray,
        const Vector3Dd &origin,
        const Vector3Dd &direction,
        bool sharesRaySpace,
        RaySharedCache &cache,
        int viewpointSlot,
        double *depth1,
        double *depth2,
        double &polyA,
        double &polyB,
        double &polyC,
        bool &trueMiss);

private:
    static void mixVectorTerms(Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c);
};

#endif

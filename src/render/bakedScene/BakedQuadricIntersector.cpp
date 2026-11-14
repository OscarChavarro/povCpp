
#include "environment/geometry/trace/QuadricTrace.h"
#include "render/bakedScene/BakedQuadricIntersector.h"

#include "java/lang/Math.h"
#include "vsdk/toolkit/environment/geometry/volume/QuadricRayExpansion.h"
#include "vsdk/toolkit/environment/geometry/volume/QuadricSurface.h"
#include "environment/geometry/element/GeometryConfig.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

// The ray products of the quadric expansion: the ray's own cached products
// when the operand shares the ray's space, otherwise those of origin and
// direction.
void
BakedQuadricIntersector::expand(
    const Quadric &shape,
    RayWithTracingState *ray,
    const Vector3Dd &origin,
    const Vector3Dd &direction,
    bool sharesRaySpace,
    RaySharedCache &cache,
    int viewpointSlot,
    double &squareTerm,
    double &linearTerm,
    double &constantTerm)
{
    Vector3Dd position2;
    Vector3Dd direction2;
    Vector3Dd positionDirection;
    Vector3Dd mixedPositionPosition;
    Vector3Dd mixedDirectionDirection;
    Vector3Dd mixedPositionDirection;
    if (sharesRaySpace) {
        // origin/direction are ray->getOrigin()/getDirection() verbatim (no
        // per-operand transform): reuse the ray's own product cache
        // (RayWithTracingState::makeRay, reset on every new ray generation)
        // instead of recomputing the same six vectors per operand.
        if (!ray->areQuadricConstantsCached()) {
            ray->makeRay();
        }
        position2 = ray->getPosition2();
        direction2 = ray->getDirection2();
        positionDirection = ray->getPositionDirection();
        mixedPositionPosition = ray->getMixedPositionPosition();
        mixedDirectionDirection = ray->getMixedDirectionDirection();
        mixedPositionDirection = ray->getMixedPositionDirection();
    } else {
        position2 = origin.multiply(origin);
        direction2 = direction.multiply(direction);
        positionDirection = origin.multiply(direction);
        mixedPositionPosition = QuadricRayExpansion::mixedProducts(origin, origin);
        mixedDirectionDirection = QuadricRayExpansion::mixedProducts(direction, direction);
        mixedPositionDirection = QuadricRayExpansion::symmetricMixedProducts(origin, direction);
    }
    const QuadricSurface &surface = shape.getSurface();
    squareTerm = QuadricRayExpansion::squareTerm(surface, shape.hasNonZeroSquareTerm(),
        direction2, mixedDirectionDirection);
    linearTerm = QuadricRayExpansion::linearTerm(surface, positionDirection, direction,
        mixedPositionDirection);
    double withoutMixed;
    if (!ray->isPrimaryRayEnabled() ||
        !cache.getQuadricViewpointConstant(viewpointSlot, withoutMixed)) {
        withoutMixed = QuadricRayExpansion::constantTermWithoutMixed(surface, position2, origin);
        if (ray->isPrimaryRayEnabled()) {
            cache.setQuadricViewpointConstant(viewpointSlot, withoutMixed);
        }
    }
    constantTerm = QuadricRayExpansion::constantTerm(surface, withoutMixed, mixedPositionPosition);
}

bool
BakedQuadricIntersector::intersectBakedQuadric(
    const Quadric &shape,
    RayWithTracingState *ray,
    const Vector3Dd &origin,
    const Vector3Dd &direction,
    bool sharesRaySpace,
    RaySharedCache &cache,
    int viewpointSlot,
    double *depth1,
    double *depth2)
{
    double polyA, polyB, polyC;
    bool trueMiss;
    return intersectBakedQuadricWithCoeffs(shape, ray, origin, direction, sharesRaySpace,
        cache, viewpointSlot, depth1, depth2, polyA, polyB, polyC, trueMiss);
}

bool
BakedQuadricIntersector::intersectBakedQuadricWithTrueMiss(
    const Quadric &shape,
    RayWithTracingState *ray,
    const Vector3Dd &origin,
    const Vector3Dd &direction,
    bool sharesRaySpace,
    RaySharedCache &cache,
    int viewpointSlot,
    double *depth1,
    double *depth2,
    bool &trueMiss)
{
    double polyA, polyB, polyC;
    return intersectBakedQuadricWithCoeffs(shape, ray, origin, direction, sharesRaySpace,
        cache, viewpointSlot, depth1, depth2, polyA, polyB, polyC, trueMiss);
}

bool
BakedQuadricIntersector::intersectBakedQuadricWithCoeffs(
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
    bool &trueMiss)
{
    GeometryStatistics &stats = *ray->getGeometryStatistics();
    stats.incrementRayQuadricTests();
    trueMiss = false;
    expand(shape, ray, origin, direction, sharesRaySpace, cache, viewpointSlot,
        polyA, polyB, polyC);
    if (!QuadricSurface::solve(polyA, polyB, polyC, *depth1, *depth2)) {
        if (polyA != 0.0) {
            // Safe to skip plane-candidate containment checks only when the ray misses
            // the quadric by more than SMALL_TOLERANCE. The containment test accepts
            // points with quadric-value < SMALL_TOLERANCE; the minimum quadric value
            // along the ray is -discriminant/(4*polyA), so trueMiss is safe exactly
            // when discriminant < -4*polyA*SMALL_TOLERANCE.
            trueMiss = polyB * polyB - 4.0 * polyA * polyC <
                -4.0 * polyA * GeometryConfig::SMALL_TOLERANCE;
        }
        return false;
    }
    if (!QuadricTrace::clipToRenderDepths(depth1, depth2)) {
        return false;
    }
    stats.incrementRayQuadricTestsSucceeded();
    return true;
}

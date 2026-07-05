#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

#include "render/bakedScene/BakedQuadricIntersector.h"

#include "java/lang/Math.h"
#include "common/Config.h"
#include "vsdk/toolkit/common/statistics/GeometryStatistics.h"

void
BakedQuadricIntersector::mixVectorTerms(Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c)
{
    a = Vector3Dd(b.x() * c.y(), b.x() * c.z(), b.y() * c.z());
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
    GeometryStatistics &stats = *ray->getGeometryStatistics();
    stats.incrementRayQuadricTests();

    Vector3Dd position2;
    Vector3Dd direction2;
    Vector3Dd positionDirection;
    Vector3Dd mixedPositionPosition;
    Vector3Dd mixedDirectionDirection;
    Vector3Dd mixedPositionDirection;
    if (sharesRaySpace) {
        // origin/direction are ray->getOrigin()/getDirection() verbatim (no
        // per-operand transform): reuse the ray's own aggregate cache
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
        Vector3Dd tempMixed;
        mixVectorTerms(mixedPositionPosition, origin, origin);
        mixVectorTerms(mixedDirectionDirection, direction, direction);
        mixVectorTerms(tempMixed, origin, direction);
        mixVectorTerms(mixedPositionDirection, direction, origin);
        mixedPositionDirection = mixedPositionDirection.add(tempMixed);
    }

    double squareTerm;
    if (shape.hasNonZeroSquareTerm()) {
        squareTerm = shape.getObject2Terms().dotProduct(direction2);
        squareTerm += shape.getObjectMixedTerms().dotProduct(mixedDirectionDirection);
    } else {
        squareTerm = 0.0;
    }

    double linearTerm = shape.getObject2Terms().dotProduct(positionDirection);
    linearTerm *= 2.0;
    linearTerm += shape.getObjectTerms().dotProduct(direction);
    linearTerm += shape.getObjectMixedTerms().dotProduct(mixedPositionDirection);

    double constantTermWithoutMixed;
    if (ray->isPrimaryRayEnabled()) {
        if (!cache.getQuadricViewpointConstant(viewpointSlot, constantTermWithoutMixed)) {
            constantTermWithoutMixed = shape.getObject2Terms().dotProduct(position2);
            constantTermWithoutMixed += shape.getObjectTerms().dotProduct(origin);
            constantTermWithoutMixed += shape.getObjectConstant();
            cache.setQuadricViewpointConstant(viewpointSlot, constantTermWithoutMixed);
        }
    } else {
        constantTermWithoutMixed = shape.getObject2Terms().dotProduct(position2);
        constantTermWithoutMixed += shape.getObjectTerms().dotProduct(origin);
        constantTermWithoutMixed += shape.getObjectConstant();
    }
    double constantTerm = constantTermWithoutMixed;
    constantTerm += shape.getObjectMixedTerms().dotProduct(mixedPositionPosition);

    if (squareTerm != 0.0) {
        const double determinant2 =
            linearTerm * linearTerm - 4.0 * squareTerm * constantTerm;
        if (determinant2 < 0.0) {
            return false;
        }

        const double determinant = java::Math::sqrt(determinant2);
        const double a2 = squareTerm * 2.0;
        const double bMinus = linearTerm * -1.0;
        *depth1 = (bMinus + determinant) / a2;
        *depth2 = (bMinus - determinant) / a2;
    } else {
        if (linearTerm == 0.0) {
            return false;
        }
        *depth1 = constantTerm * -1.0 / linearTerm;
        *depth2 = *depth1;
    }

    if ((*depth1 < Config::SMALL_TOLERANCE) ||
        (*depth1 > Config::MAX_DISTANCE)) {
        if ((*depth2 < Config::SMALL_TOLERANCE) ||
            (*depth2 > Config::MAX_DISTANCE)) {
            return false;
        }
        *depth1 = *depth2;
    } else if ((*depth2 < Config::SMALL_TOLERANCE) ||
               (*depth2 > Config::MAX_DISTANCE)) {
        *depth2 = *depth1;
    }

    stats.incrementRayQuadricTestsSucceeded();
    return true;
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
    GeometryStatistics &stats = *ray->getGeometryStatistics();
    stats.incrementRayQuadricTests();

    Vector3Dd position2;
    Vector3Dd direction2;
    Vector3Dd positionDirection;
    Vector3Dd mixedPositionPosition;
    Vector3Dd mixedDirectionDirection;
    Vector3Dd mixedPositionDirection;
    if (sharesRaySpace) {
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
        Vector3Dd tempMixed;
        mixVectorTerms(mixedPositionPosition, origin, origin);
        mixVectorTerms(mixedDirectionDirection, direction, direction);
        mixVectorTerms(tempMixed, origin, direction);
        mixVectorTerms(mixedPositionDirection, direction, origin);
        mixedPositionDirection = mixedPositionDirection.add(tempMixed);
    }

    trueMiss = false;

    double squareTerm;
    if (shape.hasNonZeroSquareTerm()) {
        squareTerm = shape.getObject2Terms().dotProduct(direction2);
        squareTerm += shape.getObjectMixedTerms().dotProduct(mixedDirectionDirection);
    } else {
        squareTerm = 0.0;
    }

    double linearTerm = shape.getObject2Terms().dotProduct(positionDirection);
    linearTerm *= 2.0;
    linearTerm += shape.getObjectTerms().dotProduct(direction);
    linearTerm += shape.getObjectMixedTerms().dotProduct(mixedPositionDirection);

    double constantTermWithoutMixed;
    if (ray->isPrimaryRayEnabled()) {
        if (!cache.getQuadricViewpointConstant(viewpointSlot, constantTermWithoutMixed)) {
            constantTermWithoutMixed = shape.getObject2Terms().dotProduct(position2);
            constantTermWithoutMixed += shape.getObjectTerms().dotProduct(origin);
            constantTermWithoutMixed += shape.getObjectConstant();
            cache.setQuadricViewpointConstant(viewpointSlot, constantTermWithoutMixed);
        }
    } else {
        constantTermWithoutMixed = shape.getObject2Terms().dotProduct(position2);
        constantTermWithoutMixed += shape.getObjectTerms().dotProduct(origin);
        constantTermWithoutMixed += shape.getObjectConstant();
    }
    double constantTerm = constantTermWithoutMixed;
    constantTerm += shape.getObjectMixedTerms().dotProduct(mixedPositionPosition);

    if (squareTerm != 0.0) {
        const double determinant2 = linearTerm * linearTerm - 4.0 * squareTerm * constantTerm;
        if (determinant2 < 0.0) {
            trueMiss = determinant2 < -4.0 * squareTerm * Config::SMALL_TOLERANCE;
            return false;
        }
        const double determinant = java::Math::sqrt(determinant2);
        const double a2 = squareTerm * 2.0;
        const double bMinus = linearTerm * -1.0;
        *depth1 = (bMinus + determinant) / a2;
        *depth2 = (bMinus - determinant) / a2;
    } else {
        if (linearTerm == 0.0) {
            return false;
        }
        *depth1 = constantTerm * -1.0 / linearTerm;
        *depth2 = *depth1;
    }

    if ((*depth1 < Config::SMALL_TOLERANCE) || (*depth1 > Config::MAX_DISTANCE)) {
        if ((*depth2 < Config::SMALL_TOLERANCE) || (*depth2 > Config::MAX_DISTANCE)) {
            return false;
        }
        *depth1 = *depth2;
    } else if ((*depth2 < Config::SMALL_TOLERANCE) || (*depth2 > Config::MAX_DISTANCE)) {
        *depth2 = *depth1;
    }

    stats.incrementRayQuadricTestsSucceeded();
    return true;
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

    Vector3Dd position2;
    Vector3Dd direction2;
    Vector3Dd positionDirection;
    Vector3Dd mixedPositionPosition;
    Vector3Dd mixedDirectionDirection;
    Vector3Dd mixedPositionDirection;
    if (sharesRaySpace) {
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
        Vector3Dd tempMixed;
        mixVectorTerms(mixedPositionPosition, origin, origin);
        mixVectorTerms(mixedDirectionDirection, direction, direction);
        mixVectorTerms(tempMixed, origin, direction);
        mixVectorTerms(mixedPositionDirection, direction, origin);
        mixedPositionDirection = mixedPositionDirection.add(tempMixed);
    }

    trueMiss = false;

    if (shape.hasNonZeroSquareTerm()) {
        polyA = shape.getObject2Terms().dotProduct(direction2);
        polyA += shape.getObjectMixedTerms().dotProduct(mixedDirectionDirection);
    } else {
        polyA = 0.0;
    }

    polyB = shape.getObject2Terms().dotProduct(positionDirection);
    polyB *= 2.0;
    polyB += shape.getObjectTerms().dotProduct(direction);
    polyB += shape.getObjectMixedTerms().dotProduct(mixedPositionDirection);

    double polyCWithoutMixed;
    if (ray->isPrimaryRayEnabled()) {
        if (!cache.getQuadricViewpointConstant(viewpointSlot, polyCWithoutMixed)) {
            polyCWithoutMixed = shape.getObject2Terms().dotProduct(position2);
            polyCWithoutMixed += shape.getObjectTerms().dotProduct(origin);
            polyCWithoutMixed += shape.getObjectConstant();
            cache.setQuadricViewpointConstant(viewpointSlot, polyCWithoutMixed);
        }
    } else {
        polyCWithoutMixed = shape.getObject2Terms().dotProduct(position2);
        polyCWithoutMixed += shape.getObjectTerms().dotProduct(origin);
        polyCWithoutMixed += shape.getObjectConstant();
    }
    polyC = polyCWithoutMixed;
    polyC += shape.getObjectMixedTerms().dotProduct(mixedPositionPosition);

    if (polyA != 0.0) {
        const double determinant2 = polyB * polyB - 4.0 * polyA * polyC;
        if (determinant2 < 0.0) {
            // Safe to skip plane-candidate containment checks only when the ray misses
            // the quadric by more than SMALL_TOLERANCE. The containment test accepts
            // points with quadric-value < SMALL_TOLERANCE; the minimum quadric value
            // along the ray is -discriminant2/(4*polyA), so trueMiss is safe exactly
            // when -discriminant2/(4*polyA) > SMALL_TOLERANCE, i.e.
            // discriminant2 < -4*polyA*SMALL_TOLERANCE.
            trueMiss = determinant2 < -4.0 * polyA * Config::SMALL_TOLERANCE;
            return false;
        }

        const double determinant = java::Math::sqrt(determinant2);
        const double a2 = polyA * 2.0;
        const double bMinus = polyB * -1.0;
        *depth1 = (bMinus + determinant) / a2;
        *depth2 = (bMinus - determinant) / a2;
    } else {
        if (polyB == 0.0) {
            return false;
        }
        *depth1 = polyC * -1.0 / polyB;
        *depth2 = *depth1;
    }

    if ((*depth1 < Config::SMALL_TOLERANCE) ||
        (*depth1 > Config::MAX_DISTANCE)) {
        if ((*depth2 < Config::SMALL_TOLERANCE) ||
            (*depth2 > Config::MAX_DISTANCE)) {
            return false;
        }
        *depth1 = *depth2;
    } else if ((*depth2 < Config::SMALL_TOLERANCE) ||
               (*depth2 > Config::MAX_DISTANCE)) {
        *depth2 = *depth1;
    }

    stats.incrementRayQuadricTestsSucceeded();
    return true;
}

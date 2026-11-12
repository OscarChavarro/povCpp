#include "java/lang/Math.h"
#include "vsdk/toolkit/environment/raycasting/geometry/boundingVolumeHierarchy/AabbBoundingVolume.h"
#include "vsdk/toolkit/environment/raycasting/geometry/element/GeometryConfig.h"
#include "vsdk/toolkit/environment/raycasting/geometry/element/IntersectionCandidate.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/Quadric.h"
#include "vsdk/toolkit/environment/geometry/volume/QuadricSurface.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "java/util/PriorityQueue.txx"

Quadric::Quadric() :
    Quadric(Vector3Dd(1.0, 1.0, 1.0), Vector3Dd(0.0, 0.0, 0.0),
        Vector3Dd(0.0, 0.0, 0.0), 1.0)
{
}

Quadric::Quadric(const Vector3Dd &object2Terms,
    const Vector3Dd &objectMixedTerms,
    const Vector3Dd &objectTerms, double objectConstant) :
    Quadric(object2Terms, objectMixedTerms, objectTerms, objectConstant,
        HUGE_VAL, false, false)
{
    updateSquareTermFlag();
}

Quadric::Quadric(const Vector3Dd &object2Terms,
    const Vector3Dd &objectMixedTerms,
    const Vector3Dd &objectTerms, double objectConstant,
    double objectVpConstant, bool constantCached, bool nonZeroSquareTerm) :
    surface(object2Terms, objectMixedTerms, objectTerms, objectConstant),
    objectVpConstant(objectVpConstant),
    constantCached(constantCached),
    nonZeroSquareTerm(nonZeroSquareTerm)
{
}

void
Quadric::updateSquareTermFlag()
{
    nonZeroSquareTerm =
        !((surface.squareCoefficients().x() == 0.0) &&
            (surface.squareCoefficients().y() == 0.0) &&
            (surface.squareCoefficients().z() == 0.0) &&
            (surface.mixedCoefficients().x() == 0.0) &&
            (surface.mixedCoefficients().y() == 0.0) &&
            (surface.mixedCoefficients().z() == 0.0));
}

int
Quadric::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    Quadric * const shape = this;
    double depth1;
    double depth2;
    Vector3Dd intersectionPoint;

    bool intersectionFound = false;
    if (Quadric::intersectQuadric(ray, shape, &depth1, &depth2)) {
        // Construct the candidate only on a real hit: ~89% of quadric tests miss
        // (drum cylinders), so deferring the ~168-byte zeroing past the test
        // removes it from the dominant miss path.
        IntersectionCandidate localElement;
        localElement.getIntersection().t = depth1;
        intersectionPoint = ray->getDirection().multiply(depth1);
        intersectionPoint = intersectionPoint.add(ray->getOrigin());
        localElement.getIntersection().point = intersectionPoint;
        localElement.getAttributes().setHitGeometry(shape);
        localElement.getAttributes().setMaterial(materialOverride);
        depthQueue->offer(localElement);
        intersectionFound = true;

        if (depth2 != depth1) {
            localElement.getIntersection().t = depth2;
            intersectionPoint = ray->getDirection().multiply(depth2);
            intersectionPoint = intersectionPoint.add(ray->getOrigin());
            localElement.getIntersection().point = intersectionPoint;
            localElement.getAttributes().setHitGeometry(shape);
            localElement.getAttributes().setMaterial(materialOverride);
            depthQueue->offer(localElement);
            intersectionFound = true;
        }
    }
    return (intersectionFound);
}

int
Quadric::doIntersectionForAllRayCrossingsAnnotated(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    Quadric * const shape = this;
    double depth1;
    double depth2;

    if (!Quadric::intersectQuadric(ray, shape, &depth1, &depth2)) {
        return false;
    }

    IntersectionCandidate localElement;
    localElement.getAttributes().setHitGeometry(shape);
    localElement.getAttributes().setMaterial(context.getMaterialOverride());
    localElement.getAttributes().pushDetailOwner(context.getDetailOwner());
    localElement.getAttributes().setMaterialUsesObjectLocalPoint(
        context.getMaterialUsesObjectLocalPoint());

    localElement.getIntersection().t = depth1;
    localElement.getIntersection().point =
        ray->getDirection().multiply(depth1).add(ray->getOrigin());
    depthQueue->offer(localElement);

    if (depth2 != depth1) {
        localElement.getIntersection().t = depth2;
        localElement.getIntersection().point =
            ray->getDirection().multiply(depth2).add(ray->getOrigin());
        depthQueue->offer(localElement);
    }

    return true;
}

bool
Quadric::doIntersectionFirstHit(
    RayWithTracingState *ray,
    IntersectionCandidate &out,
    Material *materialOverride)
{
    Quadric * const shape = this;
    double depth1;
    double depth2;
    if (!Quadric::intersectQuadric(ray, shape, &depth1, &depth2)) {
        return false;
    }

    const double nearestDepth = depth1 < depth2 ? depth1 : depth2;
    out.getIntersection().t = nearestDepth;
    out.getIntersection().point =
        ray->getDirection().multiply(nearestDepth).add(ray->getOrigin());
    out.getAttributes().setHitGeometry(shape);
    out.getAttributes().setMaterial(materialOverride);
    return true;
}

int
Quadric::intersectQuadric(
    RayWithTracingState *ray, Quadric *shape, double *depth1, double *depth2)
{
    double squareTerm;
    double linearTerm;
    double constantTerm;
    double tempTerm;
    GeometryStatistics &stats = *ray->getGeometryStatistics();

    stats.incrementRayQuadricTests();
    if (!ray->areQuadricConstantsCached()) {
        ray->makeRay();
    }

    if (shape->nonZeroSquareTerm) {
        squareTerm = shape->surface.squareCoefficients().dotProduct(ray->getDirection2());
        tempTerm =
            shape->surface.mixedCoefficients().dotProduct(ray->getMixedDirectionDirection());
        squareTerm += tempTerm;
    } else {
        squareTerm = 0.0;
    }

    linearTerm = shape->surface.squareCoefficients().dotProduct(ray->getPositionDirection());
    linearTerm *= 2.0;
    tempTerm = shape->surface.linearCoefficients().dotProduct(ray->getDirection());
    linearTerm += tempTerm;
    tempTerm =
        shape->surface.mixedCoefficients().dotProduct(ray->getMixedPositionDirection());
    linearTerm += tempTerm;

    if (ray->isPrimaryRayEnabled()) {
        if (!shape->constantCached) {
            constantTerm = shape->surface.squareCoefficients().dotProduct(ray->getPosition2());
            tempTerm = shape->surface.linearCoefficients().dotProduct(ray->getOrigin());
            constantTerm += tempTerm + shape->surface.constantCoefficient();
            shape->objectVpConstant = constantTerm;
            shape->constantCached = true;
        } else {
            constantTerm = shape->objectVpConstant;
        }
    } else {
        constantTerm = shape->surface.squareCoefficients().dotProduct(ray->getPosition2());
        tempTerm = shape->surface.linearCoefficients().dotProduct(ray->getOrigin());
        constantTerm += tempTerm + shape->surface.constantCoefficient();
    }

    tempTerm = shape->surface.mixedCoefficients().dotProduct(ray->getMixedPositionPosition());
    constantTerm += tempTerm;

    if (!QuadricSurface::solve(squareTerm, linearTerm, constantTerm, *depth1, *depth2)) {
        return false;
    }

    if ((*depth1 < GeometryConfig::SMALL_TOLERANCE) || (*depth1 > GeometryConfig::MAX_DISTANCE)) {
        if ((*depth2 < GeometryConfig::SMALL_TOLERANCE) || (*depth2 > GeometryConfig::MAX_DISTANCE)) {
            return (false);
        }
        *depth1 = *depth2;

    } else if ((*depth2 < GeometryConfig::SMALL_TOLERANCE) || (*depth2 > GeometryConfig::MAX_DISTANCE)) {
        *depth2 = *depth1;
    }

    stats.incrementRayQuadricTestsSucceeded();
    return (true);
}

int
Quadric::doContainmentTest(const Vector3Dd &testPoint, double distanceTolerance)
{
    return surface.evaluate(testPoint) < distanceTolerance ? INSIDE : OUTSIDE;
}

void
Quadric::computeSurfaceNormal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    *result = surface.gradient(*intersectionPoint);

    const double len = java::Math::sqrt(result->x() * result->x() +
        result->y() * result->y() + result->z() * result->z());
    if (len == 0.0) {
        // The normal is not defined at this point of the surface.  Set it
        // to any arbitrary direction
        *result = Vector3Dd(1.0, 0.0, 0.0);
    } else {
        // Normalize the normal
        *result = Vector3Dd(
            result->x() / len, result->y() / len, result->z() / len);
    }
}

void *
Quadric::copy()
{
    return new Quadric(*this);
}

BoundingVolumeHierarchy *
Quadric::createBoundingVolume() const
{
    GeometryBounds bounds;
    surface.getBounds(bounds);
    if (bounds.getKind() == GeometryBounds::Kind::Finite)
        return new AabbBoundingVolume(bounds.getMinimum(), bounds.getMaximum());
    return new AabbBoundingVolume(AxisAlignedBoundingBox::unbounded());
}

void
Quadric::invertGeometry()
{
    Quadric * const shape = this;

    shape->surface.squareCoefficients() = shape->surface.squareCoefficients().multiply(-1.0);
    shape->surface.mixedCoefficients() = shape->surface.mixedCoefficients().multiply(-1.0);
    shape->surface.linearCoefficients() = shape->surface.linearCoefficients().multiply(-1.0);
    shape->surface.constantCoefficient() *= -1.0;
    shape->constantCached = false;
}

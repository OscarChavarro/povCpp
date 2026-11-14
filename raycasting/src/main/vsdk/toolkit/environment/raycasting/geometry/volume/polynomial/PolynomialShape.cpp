#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingQuery.h"
#include "java/lang/Math.h"

#include "vsdk/toolkit/environment/raycasting/geometry/element/GeometryConfig.h"
#include "vsdk/toolkit/environment/raycasting/geometry/element/IntersectionCandidate.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/polynomial/PolynomialShape.h"
#include "java/util/PriorityQueue.txx"

static constexpr double SHADOW_ROOT_MIN_DISTANCE = 0.05;

PolynomialShape::PolynomialShape(int initialOrder) :
    PolynomialShape(initialOrder, 0)
{
}

PolynomialShape::PolynomialShape(int initialOrder, int sturmFlagValue) :
    surface(initialOrder, sturmFlagValue != 0),
    inverted(false)
{
}

PolynomialShape::PolynomialShape(const PolynomialShape &other) :
    surface(other.surface),
    inverted(other.inverted)
{
}

PolynomialShape::~PolynomialShape()
{
}

int
PolynomialShape::traceCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride,
    const GeometryIntersectionEmissionContext *context)
{
    double depths[PolynomialSurface::MAXIMUM_ORDER];
    Vector3Dd direction = ray->getDirection();
    const double len = java::Math::sqrt(direction.x() * direction.x() +
        direction.y() * direction.y() + direction.z() * direction.z());
    if (len == 0.0) {
        return 0;
    }
    direction = Vector3Dd(direction.x() / len, direction.y() / len, direction.z() / len);

    GeometryStatistics &stats = *ray->getGeometryStatistics();
    stats.incrementRayPolyTests();
    const int cnt = surface.intersect(ray->getOrigin(), direction,
        ray->isShadowRayEnabled() ? SHADOW_ROOT_MIN_DISTANCE : 0.0, depths);
    if (cnt > 0) {
        stats.incrementRayPolyTestsSucceeded();
    }

    bool intersectionFound = false;
    IntersectionCandidate localElement;
    for (int i = 0; i < cnt; i++) {
        if (depths[i] < 0) {
            continue;
        }
        bool repeated = false;
        for (int j = 0; j < i; j++) {
            if (depths[i] == depths[j]) {
                repeated = true;
                break;
            }
        }
        if (repeated) {
            continue;
        }
        const Vector3Dd intersectionPoint =
            direction.multiply(depths[i]).add(ray->getOrigin());
        localElement.getIntersection().t =
            intersectionPoint.subtract(ray->getOrigin()).length();
        localElement.getIntersection().point = intersectionPoint;
        localElement.getAttributes().setHitGeometry(this);
        if (context == nullptr) {
            localElement.getAttributes().setMaterial(materialOverride);
        } else {
            localElement.getAttributes().setMaterial(context->getMaterialOverride());
            localElement.getAttributes().pushDetailOwner(context->getDetailOwner());
            localElement.getAttributes().setMaterialUsesObjectLocalPoint(
                context->getMaterialUsesObjectLocalPoint());
        }
        depthQueue->offer(localElement);
        intersectionFound = true;
    }
    return intersectionFound;
}

int
PolynomialShape::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    return traceCrossings(ray, depthQueue, materialOverride, nullptr);
}

int
PolynomialShape::doIntersectionForAllRayCrossingsAnnotated(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    return traceCrossings(ray, depthQueue, nullptr, &context);
}

int
PolynomialShape::doContainmentTest(const Vector3Dd &testPoint, double distanceTolerance)
{
    double result = surface.evaluate(testPoint);
    // Points really near the surface are considered inside it.
    if (result > -GeometryConfig::INTERSECTION_EPSILON &&
        result < GeometryConfig::INTERSECTION_EPSILON) {
        result = 0.0;
    }
    if (result < distanceTolerance) {
        return inverted ? OUTSIDE : INSIDE;
    }
    return inverted ? INSIDE : OUTSIDE;
}

void
PolynomialShape::computeSurfaceNormal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    *result = surface.gradient(*intersectionPoint);
    const double vTemp = java::Math::sqrt(
        result->x() * result->x() + result->y() * result->y() +
        result->z() * result->z());
    if (vTemp > 0.0) {
        *result = Vector3Dd(
            result->x() / vTemp, result->y() / vTemp, result->z() / vTemp);
    } else {
        *result = Vector3Dd(1.0, 0.0, 0.0);
    }
    *result = (*result).normalizedFast();
}

void *
PolynomialShape::copy()
{
    return new PolynomialShape(*this);
}

PolynomialShape *
PolynomialShape::copyWithSturmFlag(int flag) const
{
    PolynomialShape * const newShape = new PolynomialShape(surface.getOrder(), flag);
    newShape->inverted = inverted;
    for (int i = 0; i < PolynomialSurface::getTermCount(surface.getOrder()); i++) {
        newShape->surface.getCoefficients()[i] = surface.getCoefficients()[i];
    }
    return newShape;
}

void
PolynomialShape::invertGeometry()
{
    this->inverted = !this->inverted;
}

RaySolidQueryStatus
PolynomialShape::classifyRay(const Ray& ray, const RayCrossingQuery& query,
    RaySolidClassification& result) const
{
    const RaySolidQueryStatus status = surface.classifyRay(ray, query, result);
    if (status == RaySolidQueryStatus::Supported && inverted) {
        for (long int i=0; i<result.insideSegments.size(); ++i)
            result.insideSegments[i] = !result.insideSegments[i];
        for (long int i=0; i<result.boundaries.size(); ++i)
            result.boundaries[i].normalReversed = !result.boundaries[i].normalReversed;
    }
    return status;
}

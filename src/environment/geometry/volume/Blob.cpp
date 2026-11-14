#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingQuery.h"
#include "java/lang/Math.h"
#include "environment/geometry/boundingVolumeHierarchy/AabbBoundingVolume.h"
#include "environment/geometry/element/GeometryConfig.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/volume/Blob.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

constexpr double Blob::INSIDE_TOLERANCE;
constexpr double Blob::SHADOW_ROOT_MIN_DISTANCE;
constexpr double Blob::INFLUENCE_START;

Blob::Blob(const BlobSolid &solidValue) :
    solid(solidValue),
    inverted(false)
{
}

Blob::Blob(const Blob &other) :
    solid(other.solid),
    inverted(other.inverted)
{
}

Blob::~Blob()
{
}

int
Blob::traceCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride,
    const GeometryIntersectionEmissionContext *context)
{
    GeometryStatistics &stats = *ray->getGeometryStatistics();
    stats.incrementRayBlobTests();
    const Vector3Dd p = ray->getOrigin();
    Vector3Dd d = ray->getDirection();
    const double len = java::Math::sqrt(d.x() * d.x() + d.y() * d.y() + d.z() * d.z());
    if (len == 0.0) {
        return 0;
    }
    d = Vector3Dd(d.x() / len, d.y() / len, d.z() / len);
    java::ArrayList<double> distances;
    solid.intersect(p, d, ray->isShadowRayEnabled() ? SHADOW_ROOT_MIN_DISTANCE : 0.0,
        INFLUENCE_START, distances);
    IntersectionCandidate localElement;
    for (long int j = 0; j < distances.size(); j++) {
        const Vector3Dd intersectionPoint = d.multiply(distances[j]).add(p);
        localElement.getIntersection().t = intersectionPoint.subtract(ray->getOrigin()).length();
        localElement.getIntersection().point = intersectionPoint;
        localElement.getAttributes().setHitGeometry(this);
        localElement.getAttributes().setMaterial(
            context != nullptr ? context->getMaterialOverride() : materialOverride);
        if (context != nullptr) {
            localElement.getAttributes().pushDetailOwner(context->getDetailOwner());
            localElement.getAttributes().setMaterialUsesObjectLocalPoint(
                context->getMaterialUsesObjectLocalPoint());
        }
        depthQueue->offer(localElement);
    }
    if (!distances.isEmpty()) {
        stats.incrementRayBlobTestsSucceeded();
    }
    return !distances.isEmpty();
}

int
Blob::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    return traceCrossings(ray, depthQueue, materialOverride, nullptr);
}

int
Blob::doIntersectionForAllRayCrossingsAnnotated(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    return traceCrossings(ray, depthQueue, context.getMaterialOverride(), &context);
}

int
Blob::doContainmentTest(const Vector3Dd &testPoint, double distanceTolerance)
{
    (void)distanceTolerance;
    // INSIDE_TOLERANCE is a density margin of the blob field, not the
    // geometric point-to-surface tolerance used elsewhere.
    if (solid.evaluateDensity(testPoint) > solid.getThreshold() - INSIDE_TOLERANCE) {
        return inverted ? OUTSIDE : INSIDE;
    }
    return inverted ? INSIDE : OUTSIDE;
}

void
Blob::computeSurfaceNormal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    *result = solid.gradient(*intersectionPoint);
    double val = (result->x() * result->x() + result->y() * result->y() +
        result->z() * result->z());
    if (val < GeometryConfig::INTERSECTION_EPSILON) {
        *result = Vector3Dd(1.0, 0.0, 0.0);
    } else {
        val = 1.0 / java::Math::sqrt(val);
        *result = (*result).multiply(val);
    }
    *result = (*result).normalizedFast();
}

void *
Blob::copy()
{
    return new Blob(*this);
}

Blob *
Blob::copyWithSturmFlag(int flag) const
{
    BlobSolid copy(solid.getThreshold(), flag != 0);
    for (long int i = 0; i < solid.getComponentCount(); i++) {
        copy.addComponent(solid.getComponent(i));
    }
    Blob *result = new Blob(copy);
    result->inverted = inverted;
    return result;
}

void
Blob::invertGeometry()
{
    this->inverted = !this->inverted;
}

BoundingVolumeHierarchy *
Blob::createBoundingVolume() const
{
    if (solid.getComponentCount() == 0) {
        return new AabbBoundingVolume(AxisAlignedBoundingBox::unbounded());
    }
    // Local-space AABB from all component centers +/- radius
    AxisAlignedBoundingBox local = AxisAlignedBoundingBox::empty();
    for (long int i = 0; i < solid.getComponentCount(); i++) {
        const double r = java::Math::sqrt(solid.getComponent(i).getRadiusSquared());
        const Vector3Dd &c = solid.getComponent(i).getCenter();
        local = local.expandedBy(Vector3Dd(c.x() - r, c.y() - r, c.z() - r));
        local = local.expandedBy(Vector3Dd(c.x() + r, c.y() + r, c.z() + r));
    }
    return new AabbBoundingVolume(
        AxisAlignedBoundingBox::fromTransformedCorners(local.getMin(), local.getMax(), nullptr));
}

RaySolidQueryStatus
Blob::classifyRay(const Ray& ray, const RayCrossingQuery& query,
    RaySolidClassification& result) const
{
    const RaySolidQueryStatus status = solid.classifyRay(ray, query, result);
    if (status == RaySolidQueryStatus::Supported && inverted) {
        for (long int i=0; i<result.insideSegments.size(); ++i)
            result.insideSegments[i] = !result.insideSegments[i];
        for (long int i=0; i<result.boundaries.size(); ++i)
            result.boundaries[i].normalReversed = !result.boundaries[i].normalReversed;
    }
    return status;
}

#include "java/lang/Math.h"
#include "environment/geometry/boundingVolumeHierarchy/AabbBoundingVolume.h"
#include "environment/geometry/element/GeometryConfig.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/volume/RayCastingBox.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "environment/geometry/volume/BoxInvertedCrossingSink.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

RayCastingBox::RayCastingBox() :
    RayCastingBox(Vector3Dd(-1.0, -1.0, -1.0), Vector3Dd(1.0, 1.0, 1.0), false)
{
}

RayCastingBox::RayCastingBox(const Vector3Dd &minBounds, const Vector3Dd &maxBounds, bool inverted) :
    box(minBounds, maxBounds),
    inverted(inverted)
{
}

RayCastingBox::RayCastingBox(const RayCastingBox &other) :
    RayCastingBox(other.getBounds()[0], other.getBounds()[1], other.inverted)
{
}

RayCastingBox::~RayCastingBox()
{
}


int
RayCastingBox::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    double depth1;
    double depth2;
    Vector3Dd intersectionPoint;
    IntersectionCandidate localElement;
    bool intersectionFound = false;
    RayCastingBox * const shape = this;
    if (RayCastingBox::intersectBoxx(ray, shape, &depth1, &depth2)) {
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
RayCastingBox::doIntersectionForAllRayCrossingsAnnotated(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    double depth1;
    double depth2;
    RayCastingBox * const shape = this;
    if (!RayCastingBox::intersectBoxx(ray, shape, &depth1, &depth2)) {
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
RayCastingBox::doIntersectionFirstHit(
    RayWithTracingState *ray,
    IntersectionCandidate &out,
    Material *materialOverride)
{
    double depth1;
    double depth2;
    RayCastingBox * const shape = this;
    if (!RayCastingBox::intersectBoxx(ray, shape, &depth1, &depth2)) {
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
RayCastingBox::intersectBoxx(
    const RayWithTracingState *ray, const RayCastingBox *box, double *depth1, double *depth2)
{
    GeometryStatistics &stats = *ray->getGeometryStatistics();
    stats.incrementRayBoxTests();
    double tmin = 0.0;
    double tmax = HUGE_VAL;
    bool faceOverlap = false;
    if (Box::intersectBounds(ray->getOrigin(), ray->getDirection(),
            box->getBounds()[0], box->getBounds()[1], GeometryConfig::INTERSECTION_EPSILON,
            false, tmin, tmax, faceOverlap) != RayFirstHitStatus::Hit) {
        return false;
    }

    *depth1 = tmin;
    *depth2 = tmax;

    if ((*depth1 < GeometryConfig::SMALL_TOLERANCE) || (*depth1 > GeometryConfig::MAX_DISTANCE)) {
        if ((*depth2 < GeometryConfig::SMALL_TOLERANCE) || (*depth2 > GeometryConfig::MAX_DISTANCE)) {
            return (false);
        }
        *depth1 = *depth2;

    } else if ((*depth2 < GeometryConfig::SMALL_TOLERANCE) || (*depth2 > GeometryConfig::MAX_DISTANCE)) {
        *depth2 = *depth1;
    }

    stats.incrementRayBoxTestsSucceeded();
    return (true);
}

int
RayCastingBox::doContainmentTest(const Vector3Dd &testPoint, double distanceTolerance)
{
    (void)distanceTolerance;
    const bool inside = Box::containsBounds(testPoint, box.getRawBounds()[0], box.getRawBounds()[1]);
    return inside != inverted ? INSIDE : OUTSIDE;
}

void
RayCastingBox::computeSurfaceNormal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    *result = Box::normalForBounds(*intersectionPoint, box.getRawBounds()[0], box.getRawBounds()[1],
        GeometryConfig::INTERSECTION_EPSILON);
    if (result->x() == 0 && result->y() == 0 && result->z() == 0) {
        *result = Vector3Dd(1,0,0);
    }
}

void *
RayCastingBox::copy()
{
    return new RayCastingBox(*this);
}

void
RayCastingBox::invertGeometry()
{
    this->inverted = !this->inverted;
}

BoundingVolumeHierarchy *
RayCastingBox::createBoundingVolume() const
{
    return new AabbBoundingVolume(box.getRawBounds()[0], box.getRawBounds()[1]);
}

void RayCastingBox::getBounds(GeometryBounds& result) const
{
    if (inverted) result.setUnbounded();
    else box.getBounds(result);
}

GeometryContainment RayCastingBox::classifyPoint(const Vector3Dd& point, double tolerance) const
{
    const GeometryContainment result = box.classifyPoint(point, tolerance);
    if (!inverted) return result;
    if (result == GeometryContainment::Inside) return GeometryContainment::Outside;
    if (result == GeometryContainment::Outside) return GeometryContainment::Inside;
    return result;
}
RayCrossingSupport RayCastingBox::emitRayCrossings(const Ray& ray,
    const RayCrossingQuery& query, RayCrossingSink& sink) const
{
    if (!inverted) return box.emitRayCrossings(ray, query, sink);
    BoxInvertedCrossingSink reversed(sink);
    return box.emitRayCrossings(ray, query, reversed);
}
RayFirstHitStatus RayCastingBox::queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
    RayHit* hit, RayBoundary* boundary) const
{
    const RayFirstHitStatus status = box.queryFirstHit(ray, query, hit, boundary);
    if (status == RayFirstHitStatus::Hit && inverted) {
        if (boundary) boundary->normalReversed = !boundary->normalReversed;
        if (hit && hit->needsNormal() && (hit->getAvailableDetailMask() & RayHit::DETAIL_NORMAL))
            hit->normal = hit->normal.multiply(-1);
    }
    return status;
}
RayFirstHitStatus RayCastingBox::querySurfaceDetail(const Ray& ray,
    const RayBoundary& boundary, RayHit* hit) const
{
    return box.querySurfaceDetail(ray, boundary, hit);
}
RaySolidQueryStatus RayCastingBox::classifyRay(const Ray& ray,
    const RayCrossingQuery& query, RaySolidClassification& result) const
{
    const RaySolidQueryStatus status = box.classifyRay(ray, query, result);
    if (status == RaySolidQueryStatus::Supported && inverted) {
        for (long int i=0; i<result.insideSegments.size(); ++i)
            result.insideSegments[i] = !result.insideSegments[i];
        for (long int i=0; i<result.boundaries.size(); ++i)
            result.boundaries[i].normalReversed = !result.boundaries[i].normalReversed;
    }
    return status;
}

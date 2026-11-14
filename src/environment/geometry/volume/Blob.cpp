#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingQuery.h"
#include "java/lang/Math.h"
#include "environment/geometry/boundingVolumeHierarchy/AabbBoundingVolume.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/volume/Blob.h"
#include "environment/geometry/trace/BlobTrace.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

Blob::Blob(const BlobSolid &solidValue) :
    solid(solidValue)
{
}

Blob::Blob(const Blob &other) :
    solid(other.solid),
    traceState(other.traceState)
{
}

Blob::~Blob()
{
}

int
Blob::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    return BlobTrace::emitCrossings(solid, ray, depthQueue, materialOverride, nullptr);
}

int
Blob::doIntersectionForAllRayCrossingsAnnotated(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    return BlobTrace::emitCrossings(solid, ray, depthQueue, context.getMaterialOverride(), &context);
}

int
Blob::doContainmentTest(const Vector3Dd &testPoint, double distanceTolerance)
{
    return BlobTrace::containment(solid, traceState, testPoint, distanceTolerance);
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
    result->traceState = traceState;
    return result;
}

void
Blob::invertGeometry()
{
    traceState.setInverted(!traceState.isInverted());
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
    if (status == RaySolidQueryStatus::Supported && traceState.isInverted()) {
        for (long int i=0; i<result.insideSegments.size(); ++i)
            result.insideSegments[i] = !result.insideSegments[i];
        for (long int i=0; i<result.boundaries.size(); ++i)
            result.boundaries[i].normalReversed = !result.boundaries[i].normalReversed;
    }
    return status;
}

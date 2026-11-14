#include "java/lang/Math.h"
#include "environment/geometry/trace/HeightFieldTrace.h"
#include "vsdk/toolkit/processing/Containment.h"
#include "java/util/PriorityQueue.txx"

int
HeightFieldTrace::emitCrossings(HeightFieldSolid &solid, RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride, const GeometryIntersectionEmissionContext *context)
{
    GeometryStatistics &stats = *ray->getGeometryStatistics();
    stats.incrementRayHtFieldTests();
    stats.incrementRayBoxTests();
    double depth = 0.0;
    const HeightFieldTraceResult result = solid.traceFirstHit(
        ray->getOrigin(), ray->getDirection(), depth);
    if (result == HeightFieldTraceResult::MissedBounds) {
        return false;
    }
    stats.incrementRayBoxTestsSucceeded();
    if (result != HeightFieldTraceResult::Hit) {
        return false;
    }
    IntersectionCandidate localElement;
    localElement.getAttributes().setMaterial(materialOverride);
    if (context != nullptr) {
        localElement.getAttributes().pushDetailOwner(context->getDetailOwner());
        localElement.getAttributes().setMaterialUsesObjectLocalPoint(
            context->getMaterialUsesObjectLocalPoint());
    }
    localElement.getIntersection().t = depth;
    localElement.getIntersection().point =
        ray->getDirection().multiply(depth).add(ray->getOrigin());
    localElement.getAttributes().setHitSurface(&solid, SurfaceDetailFamily::HeightField);
    depthQueue->offer(localElement);
    stats.incrementRayHtFieldTestsSucceeded();
    return true;
}

// PovRay 1.0 containment: the integer-truncated plane test of the triangle
// under the point, and only above one unit over the box floor.
int
HeightFieldTrace::containment(const HeightFieldSolid &solid, const Vector3Dd &point,
    double distanceTolerance)
{
    (void)distanceTolerance;
    const int inside = static_cast<int>(Containment::INSIDE);
    const int outside = static_cast<int>(Containment::OUTSIDE);
    const Vector3Dd &test = point;
    int px = (int)test.x();
    int pz = (int)test.z();
    const double x = test.x() - (double)px;
    const double z = test.z() - (double)pz;
    Vector3Dd localOrigin;
    Vector3Dd temp1;
    Vector3Dd temp2;
    if ((x + z) < 1.0) {
        if (px < 0 || pz < 0 || px + 1 >= solid.getColumnCount() || pz + 1 >= solid.getRowCount()) {
            return outside;
        }
        const double y1 = solid.getHeight(px, pz);
        const double y2 = solid.getHeight(px + 1, pz);
        const double y3 = solid.getHeight(px, pz + 1);
        localOrigin = Vector3Dd((double)px, y1, (double)pz);
        temp1 = Vector3Dd(1.0, y2 - y1, 0.0);
        temp2 = Vector3Dd(0.0, y3 - y1, 1.0);
    } else {
        px = java::Math::ceil(test.x());
        pz = java::Math::ceil(test.z());
        if (px < 1 || pz < 1 || px >= solid.getColumnCount() || pz >= solid.getRowCount()) {
            return outside;
        }
        const double y1 = solid.getHeight(px, pz);
        const double y2 = solid.getHeight(px - 1, pz);
        const double y3 = solid.getHeight(px, pz - 1);
        localOrigin = Vector3Dd((double)px, y1, (double)pz);
        temp1 = Vector3Dd(-1.0, y2 - y1, 0.0);
        temp2 = Vector3Dd(0.0, y3 - y1, -1.0);
    }
    Vector3Dd localNormal = temp2.crossProduct(temp1);
    if (localNormal.y() < 0.0) {
        localNormal = localNormal.multiply(-1.0);
    }
    const int dot1 = (int)test.dotProduct(localNormal);
    const int dot2 = (int)localOrigin.dotProduct(localNormal);
    if ((dot1 < dot2) && (test.y() > solid.getMinimum().y() + 1.0)) {
        return inside;
    }
    return outside;
}

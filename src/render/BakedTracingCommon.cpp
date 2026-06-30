#include "java/util/ArrayList.txx"

#include "render/BakedCsgTracing.h"
#include "render/BakedCompositeTracing.h"
#include "render/BakedSimpleBodyTracing.h"
#include "render/BakedTracingCommon.h"

bool
BakedTracingCommon::traceObjectFirstHit(
    const Scene::CompiledTracingObject &entry,
    const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    const java::ArrayList<Scene::BakedComposite> &bakedComposites,
    RayWithSegments *ray,
    IntersectionCandidate &out)
{
    if (entry.bakedSimpleBodyIndex >= 0) {
        return BakedSimpleBodyTracing::traceFirstHit(
            bakedSimpleBodies[entry.bakedSimpleBodyIndex],
            bakedSimpleBodies,
            bakedCsgs,
            bakedComposites,
            ray,
            out);
    }
    if (entry.bakedCompositeIndex >= 0) {
        return BakedCompositeTracing::traceFirstHit(
            bakedComposites[entry.bakedCompositeIndex],
            bakedSimpleBodies,
            bakedCsgs,
            bakedComposites,
            ray,
            out);
    }
    return entry.object != nullptr && entry.object->doIntersectionFirstHit(ray, out);
}

bool
BakedTracingCommon::traceObjectAllCrossings(
    const Scene::CompiledTracingObject &entry,
    const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    const java::ArrayList<Scene::BakedComposite> &bakedComposites,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue)
{
    if (entry.bakedSimpleBodyIndex >= 0) {
        return BakedSimpleBodyTracing::traceAllCrossings(
            bakedSimpleBodies[entry.bakedSimpleBodyIndex],
            bakedSimpleBodies,
            bakedCsgs,
            bakedComposites,
            ray,
            depthQueue);
    }
    if (entry.bakedCompositeIndex >= 0) {
        return BakedCompositeTracing::traceAllCrossings(
            bakedComposites[entry.bakedCompositeIndex],
            bakedSimpleBodies,
            bakedCsgs,
            bakedComposites,
            ray,
            depthQueue);
    }
    return entry.object != nullptr &&
        entry.object->doIntersectionForAllRayCrossings(ray, depthQueue);
}

int
BakedTracingCommon::containmentTest(
    const Scene::CompiledTracingObject &entry,
    const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    const java::ArrayList<Scene::BakedComposite> &bakedComposites,
    const Vector3Dd &point,
    double distanceTolerance)
{
    if (entry.bakedSimpleBodyIndex >= 0) {
        return BakedSimpleBodyTracing::containmentTest(
            bakedSimpleBodies[entry.bakedSimpleBodyIndex],
            bakedSimpleBodies,
            bakedCsgs,
            bakedComposites,
            point,
            distanceTolerance);
    }
    if (entry.bakedCompositeIndex >= 0) {
        return BakedCompositeTracing::containmentTest(
            bakedComposites[entry.bakedCompositeIndex],
            bakedSimpleBodies,
            bakedCsgs,
            bakedComposites,
            point,
            distanceTolerance);
    }
    if (entry.object == nullptr) {
        return Geometry::OUTSIDE;
    }
    return entry.object->doContainmentTest(point, distanceTolerance);
}

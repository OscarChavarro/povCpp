#include "java/util/ArrayList.txx"

#include <atomic>

#include "render/BakedCsgTracing.h"
#include "render/BakedCompositeTracing.h"
#include "render/BakedSimpleBodyTracing.h"
#include "render/BakedTracingCommon.h"

namespace {
std::atomic<long> firstHitObjectFallbacks(0);
std::atomic<long> allCrossingsObjectFallbacks(0);
std::atomic<long> containmentObjectFallbacks(0);
}

void
BakedTracingCommon::resetFallbackCounters()
{
    firstHitObjectFallbacks.store(0);
    allCrossingsObjectFallbacks.store(0);
    containmentObjectFallbacks.store(0);
}

BakedTracingCommon::FallbackCounters
BakedTracingCommon::getFallbackCounters()
{
    FallbackCounters counters;
    counters.firstHitObjectFallbacks = firstHitObjectFallbacks.load();
    counters.allCrossingsObjectFallbacks = allCrossingsObjectFallbacks.load();
    counters.containmentObjectFallbacks = containmentObjectFallbacks.load();
    return counters;
}

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
    // Temporary Phase 1 diagnostic: this is the remaining top-level escape
    // from finalized baked traversal into parsed SimpleBody behavior.
    firstHitObjectFallbacks.fetch_add(1);
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
    // Temporary Phase 1 diagnostic: bounding/clipping or shadow traversal
    // reached an object that does not yet have a baked execution record.
    allCrossingsObjectFallbacks.fetch_add(1);
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
    // Temporary Phase 1 diagnostic: containment still depends on parsed
    // SimpleBody ownership/detail callbacks for this entry.
    containmentObjectFallbacks.fetch_add(1);
    return entry.object->doContainmentTest(point, distanceTolerance);
}

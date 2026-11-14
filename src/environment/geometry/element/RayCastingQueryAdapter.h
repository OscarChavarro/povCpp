#ifndef POV_VITRAL_QUERY_ADAPTER_H
#define POV_VITRAL_QUERY_ADAPTER_H

#include <cmath>
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.txx"
#include "environment/geometry/element/RayWithTracingState.h"
#include "environment/geometry/element/PriorityQueuePool.txx"
#include "environment/geometry/element/RayCastingHit.h"
#include "java/util/PriorityQueue.txx"

// Compatibility workspace for callers of the shared VITRAL geometric API.
// The native POV render path continues to reuse its per-worker workspace.
class RayCastingQueryAdapter {
public:
    GeometryStatistics statistics;
    RendererConfiguration configuration;
    PriorityQueuePool<IntersectionCandidate> pool;
    RayWithTracingState ray;

    explicit RayCastingQueryAdapter(const Ray& source) {
        ray.setOriginAndDirection(source.getOrigin(), source.getDirection());
        ray.setT(source.getT());
        ray.setGeometryStatistics(&statistics);
        ray.setConfig(&configuration);
        ray.setIntersectionQueuePool(&pool);
    }

    template<class Traversable>
    bool findCrossing(Traversable* geometry, double t, IntersectionCandidate& result) {
        auto* queue = pool.pop(128);
        geometry->doIntersectionForAllRayCrossings(&ray, queue);
        bool found = false;
        double best = 1e-7 * (1.0 + std::abs(t));
        for (const IntersectionCandidate& candidate : *queue) {
            const double error = std::abs(candidate.getIntersection().t - t);
            if (error <= best) { best = error; result = candidate; found = true; }
        }
        pool.push(queue);
        return found;
    }

    static void exportHit(const Ray& source, const RayCastingHit& detail, RayHit* out) {
        if (!out) return;
        out->setHitDistance(detail.hitDistance);
        if (out->shouldStoreRay()) out->setRay(source.withT(detail.hitDistance));
        if (out->needsPoint()) out->point = detail.p;
        if (out->needsNormal()) out->normal = detail.n;
        // UV/tangent generation remains in the POV material pipeline.
    }
};
#endif

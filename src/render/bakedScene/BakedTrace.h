#ifndef __BAKED_TRACE__
#define __BAKED_TRACE__

#include "render/bakedScene/BakedScene.h"
#include "render/raySharedCache/RaySharedCache.h"

class BakedTrace {
  public:
    static bool traceFirstHit(
        const BakedScene &scene,
        int objectIndex,
        RayWithTracingState *ray,
        IntersectionCandidate &out,
        RaySharedCache &cache);

    static bool traceAllCrossings(
        const BakedScene &scene,
        int objectIndex,
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        RaySharedCache &cache);

    static int containmentTest(
        const BakedScene &scene,
        int objectIndex,
        const Vector3Dd &point,
        double distanceTolerance);

  private:
    static bool traceSimpleBodyAllCrossings(
        const BakedScene &scene,
        const TraceableObject *baked,
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        RaySharedCache &cache);

    static bool traceSimpleBodyFirstHit(
        const BakedScene &scene,
        const TraceableObject *baked,
        RayWithTracingState *ray,
        IntersectionCandidate &out,
        RaySharedCache &cache);

    static int simpleBodyContainmentTest(
        const BakedScene &scene,
        const TraceableObject *baked,
        const Vector3Dd &point,
        double distanceTolerance);

    static bool finalizeSimpleBodyCandidate(
        const BakedScene &scene,
        const TraceableObject *baked,
        RayWithTracingState *ray,
        IntersectionCandidate &candidate);

    static bool passesBoundingShapes(
        const BakedScene &scene,
        const TraceableObject *baked,
        RayWithTracingState *objectRayPtr,
        RaySharedCache &cache);

    static bool traceCompositeAllCrossings(
        const BakedScene &scene,
        const CompositeRecord *composite,
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        RaySharedCache &cache);

    static bool traceCompositeAllCrossingsInCompositeSpace(
        const BakedScene &scene,
        const CompositeRecord *composite,
        RayWithTracingState *ray,
        RayWithTracingState *compositeRayPtr,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        RaySharedCache &cache);

    static bool traceCompositeFirstHit(
        const BakedScene &scene,
        const CompositeRecord *composite,
        RayWithTracingState *ray,
        IntersectionCandidate &out,
        RaySharedCache &cache);

    static int compositeContainmentTest(
        const BakedScene &scene,
        const CompositeRecord *composite,
        const Vector3Dd &point,
        double distanceTolerance);

    static bool rayIntersectsAabbForward(
        const RayWithTracingState &ray, const AxisAlignedBoundingBox &box);

    static bool pointInsideAabb(
        const Vector3Dd &point, const AxisAlignedBoundingBox &box, double tolerance);
};

#endif

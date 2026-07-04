#ifndef __BAKED_TRACE__
#define __BAKED_TRACE__

#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/scene/Scene.h"
#include "render/bakedScene/BakedScene.h"

// Plan 6: the new trace entry points, reading BakedScene exclusively -
// Phase 2 ported the non-CSG paths (with a temporary bridge to the old CSG
// layer for Csg-kind objects); Phase 3 ported the CSG algorithms themselves
// (see BakedCsgTrace) and removed that bridge, so every TraceKind now
// resolves entirely against BakedScene with no dependency on
// Scene::CompiledTracingScene.
class BakedTrace {
  public:
    static bool traceFirstHit(
        const BakedScene &scene,
        int objectIndex,
        RayWithSegments *ray,
        IntersectionCandidate &out);

    static bool traceAllCrossings(
        const BakedScene &scene,
        int objectIndex,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue);

    static int containmentTest(
        const BakedScene &scene,
        int objectIndex,
        const Vector3Dd &point,
        double distanceTolerance);

  private:
    static bool traceSimpleBodyAllCrossings(
        const BakedScene &scene,
        const BakedScene::TraceableObject &baked,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue);

    static bool traceSimpleBodyFirstHit(
        const BakedScene &scene,
        const BakedScene::TraceableObject &baked,
        RayWithSegments *ray,
        IntersectionCandidate &out);

    static int simpleBodyContainmentTest(
        const BakedScene &scene,
        const BakedScene::TraceableObject &baked,
        const Vector3Dd &point,
        double distanceTolerance);

    static bool finalizeSimpleBodyCandidate(
        const BakedScene &scene,
        const BakedScene::TraceableObject &baked,
        RayWithSegments *ray,
        IntersectionCandidate &candidate);

    static bool passesBoundingShapes(
        const BakedScene &scene,
        const BakedScene::TraceableObject &baked,
        RayWithSegments *objectRayPtr);

    static bool traceCompositeAllCrossings(
        const BakedScene &scene,
        const BakedScene::CompositeRecord &composite,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue);

    static bool traceCompositeAllCrossingsInCompositeSpace(
        const BakedScene &scene,
        const BakedScene::CompositeRecord &composite,
        RayWithSegments *ray,
        RayWithSegments *compositeRayPtr,
        java::PriorityQueue<IntersectionCandidate> *depthQueue);

    static bool traceCompositeFirstHit(
        const BakedScene &scene,
        const BakedScene::CompositeRecord &composite,
        RayWithSegments *ray,
        IntersectionCandidate &out);

    static int compositeContainmentTest(
        const BakedScene &scene,
        const BakedScene::CompositeRecord &composite,
        const Vector3Dd &point,
        double distanceTolerance);

    static bool rayIntersectsAabbForward(
        const RayWithSegments &ray, const AxisAlignedBox &box);

    static bool pointInsideAabb(
        const Vector3Dd &point, const AxisAlignedBox &box, double tolerance);
};

#endif

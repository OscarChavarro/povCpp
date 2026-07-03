#ifndef __BAKED_TRACE__
#define __BAKED_TRACE__

#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/scene/Scene.h"
#include "render/bakedScene/BakedScene.h"

// Plan 6 Phase 2: the new trace entry points, reading BakedScene instead of
// Scene::CompiledTracingScene for DirectPrimitive/Composite/BoundedGeneric/
// GenericFallback objects. Csg-kind objects still bridge to the old
// BakedSimpleBodyTracing/BakedCsgTracing layer via `legacyScene` (same
// object indices - see BakedSceneBuilder) - Phase 3 ports the CSG algorithms
// themselves and removes this bridge and the `legacyScene` parameter.
class BakedTrace {
  public:
    static bool traceFirstHit(
        const BakedScene &scene,
        const Scene::CompiledTracingScene &legacyScene,
        int objectIndex,
        RayWithSegments *ray,
        IntersectionCandidate &out);

    static bool traceAllCrossings(
        const BakedScene &scene,
        const Scene::CompiledTracingScene &legacyScene,
        int objectIndex,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue);

    static int containmentTest(
        const BakedScene &scene,
        const Scene::CompiledTracingScene &legacyScene,
        int objectIndex,
        const Vector3Dd &point,
        double distanceTolerance);

  private:
    static bool traceSimpleBodyAllCrossings(
        const BakedScene &scene,
        const Scene::CompiledTracingScene &legacyScene,
        const BakedScene::TraceableObject &baked,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue);

    static bool traceSimpleBodyFirstHit(
        const BakedScene &scene,
        const Scene::CompiledTracingScene &legacyScene,
        const BakedScene::TraceableObject &baked,
        RayWithSegments *ray,
        IntersectionCandidate &out);

    static int simpleBodyContainmentTest(
        const BakedScene &scene,
        const Scene::CompiledTracingScene &legacyScene,
        const BakedScene::TraceableObject &baked,
        const Vector3Dd &point,
        double distanceTolerance);

    static bool finalizeSimpleBodyCandidate(
        const BakedScene &scene,
        const Scene::CompiledTracingScene &legacyScene,
        const BakedScene::TraceableObject &baked,
        RayWithSegments *ray,
        IntersectionCandidate &candidate);

    static bool passesBoundingShapes(
        const BakedScene &scene,
        const Scene::CompiledTracingScene &legacyScene,
        const BakedScene::TraceableObject &baked,
        RayWithSegments *objectRayPtr);

    static bool traceCompositeAllCrossings(
        const BakedScene &scene,
        const Scene::CompiledTracingScene &legacyScene,
        const BakedScene::CompositeRecord &composite,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue);

    static bool traceCompositeAllCrossingsInCompositeSpace(
        const BakedScene &scene,
        const Scene::CompiledTracingScene &legacyScene,
        const BakedScene::CompositeRecord &composite,
        RayWithSegments *ray,
        RayWithSegments *compositeRayPtr,
        java::PriorityQueue<IntersectionCandidate> *depthQueue);

    static bool traceCompositeFirstHit(
        const BakedScene &scene,
        const Scene::CompiledTracingScene &legacyScene,
        const BakedScene::CompositeRecord &composite,
        RayWithSegments *ray,
        IntersectionCandidate &out);

    static int compositeContainmentTest(
        const BakedScene &scene,
        const Scene::CompiledTracingScene &legacyScene,
        const BakedScene::CompositeRecord &composite,
        const Vector3Dd &point,
        double distanceTolerance);

    static bool rayIntersectsAabbForward(
        const RayWithSegments &ray, const AxisAlignedBox &box);

    static bool pointInsideAabb(
        const Vector3Dd &point, const AxisAlignedBox &box, double tolerance);
};

#endif

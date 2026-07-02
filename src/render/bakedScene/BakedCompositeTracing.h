#ifndef __BAKED_COMPOSITE_TRACING__
#define __BAKED_COMPOSITE_TRACING__

#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/scene/Scene.h"

class BakedCompositeTracing {
public:
    static bool traceFirstHit(
        const Scene::BakedComposite &bakedComposite,
        const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const java::ArrayList<Scene::BakedComposite> &bakedComposites,
        RayWithSegments *ray,
        IntersectionCandidate &out);

    static bool traceAllCrossings(
        const Scene::BakedComposite &bakedComposite,
        const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const java::ArrayList<Scene::BakedComposite> &bakedComposites,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue);

    static int containmentTest(
        const Scene::BakedComposite &bakedComposite,
        const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const java::ArrayList<Scene::BakedComposite> &bakedComposites,
        const Vector3Dd &point,
        double distanceTolerance);

private:
    static bool rayIntersectsAabbForward(
        const RayWithSegments &ray,
        const AxisAlignedBox &box);

    static bool pointInsideAabb(
        const Vector3Dd &point,
        const AxisAlignedBox &box,
        double tolerance);

    static bool traceAllCrossingsInCompositeSpace(
        const Scene::BakedComposite &bakedComposite,
        const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const java::ArrayList<Scene::BakedComposite> &bakedComposites,
        RayWithSegments *ray,
        RayWithSegments *compositeRayPtr,
        java::PriorityQueue<IntersectionCandidate> *depthQueue);
};

#endif

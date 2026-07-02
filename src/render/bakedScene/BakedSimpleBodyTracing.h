#ifndef __BAKED_SIMPLE_BODY_TRACING__
#define __BAKED_SIMPLE_BODY_TRACING__

#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/scene/Scene.h"

class BakedSimpleBodyTracing {
public:
    static bool traceFirstHit(
        const Scene::BakedSimpleBody &baked,
        const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const java::ArrayList<Scene::BakedComposite> &bakedComposites,
        RayWithSegments *ray,
        IntersectionCandidate &out);

    static bool traceAllCrossings(
        const Scene::BakedSimpleBody &baked,
        const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const java::ArrayList<Scene::BakedComposite> &bakedComposites,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue);

    static int containmentTest(
        const Scene::BakedSimpleBody &baked,
        const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const java::ArrayList<Scene::BakedComposite> &bakedComposites,
        const Vector3Dd &point,
        double distanceTolerance);

private:
    static bool finalizeCandidate(
        const Scene::BakedSimpleBody &baked,
        const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const java::ArrayList<Scene::BakedComposite> &bakedComposites,
        RayWithSegments *ray,
        IntersectionCandidate &candidate);

    static bool passesBoundingShapes(
        const Scene::BakedSimpleBody &baked,
        const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const java::ArrayList<Scene::BakedComposite> &bakedComposites,
        RayWithSegments *objectRayPtr);
};

#endif

#ifndef __BAKED_TRACING_COMMON__
#define __BAKED_TRACING_COMMON__

#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/scene/Scene.h"

namespace BakedTracingCommon {
bool traceObjectFirstHit(
    const Scene::CompiledTracingObject &entry,
    const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    const java::ArrayList<Scene::BakedComposite> &bakedComposites,
    RayWithSegments *ray,
    IntersectionCandidate &out);

bool traceObjectAllCrossings(
    const Scene::CompiledTracingObject &entry,
    const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    const java::ArrayList<Scene::BakedComposite> &bakedComposites,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue);

int containmentTest(
    const Scene::CompiledTracingObject &entry,
    const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    const java::ArrayList<Scene::BakedComposite> &bakedComposites,
    const Vector3Dd &point,
    double distanceTolerance);
}

#endif

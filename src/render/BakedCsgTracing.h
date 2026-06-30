#ifndef __BAKED_CSG_TRACING__
#define __BAKED_CSG_TRACING__

#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/material/Material.h"
#include "environment/scene/Scene.h"

namespace BakedCsgTracing {
bool traceAllCrossings(
    const Scene::BakedConstructiveSolidGeometry &bakedCsg,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride = nullptr);

bool traceFirstHit(
    const Scene::BakedConstructiveSolidGeometry &bakedCsg,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    RayWithSegments *ray,
    IntersectionCandidate &out,
    Material *materialOverride = nullptr);

int containmentTest(
    const Scene::BakedConstructiveSolidGeometry &bakedCsg,
    const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
    const Vector3Dd &point,
    double distanceTolerance);
}

#endif

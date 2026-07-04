#ifndef __BAKED_PLANE_INTERSECTOR__
#define __BAKED_PLANE_INTERSECTOR__

#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/material/Material.h"
#include "render/bakedScene/BakedScene.h"
#include "render/raySharedCache/RaySharedCache.h"

// Ray-plane intersection and containment for baked CSG plane operands
// (both direct planes and the infinite-plane fast-path candidate builders
// used by the union/single-core-plane trace paths).
class BakedPlaneIntersector {
public:
    static bool intersectBakedPlane(
        const BakedScene::CsgOperandRecord &operand,
        RayWithSegments *ray,
        const Vector3Dd &origin,
        const Vector3Dd &direction,
        RaySharedCache &cache,
        double *depth);

    static int planeContainmentTest(
        const BakedScene::CsgOperandRecord &operand,
        const Vector3Dd &point,
        double distanceTolerance);

    static bool tracePlaneOperandCandidate(
        const BakedScene::CsgOperandRecord &operand,
        RayWithSegments *ray,
        RaySharedCache &cache,
        Material *materialOverride,
        IntersectionCandidate &candidate);

    static bool tracePlaneOperandCandidateInRaySpace(
        const BakedScene::CsgOperandRecord &operand,
        RayWithSegments *statsRay,
        const Vector3Dd &rayOrigin,
        const Vector3Dd &rayDirection,
        RaySharedCache &cache,
        Material *materialOverride,
        IntersectionCandidate &candidate);
};

#endif

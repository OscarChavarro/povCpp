#ifndef __DIRECT_LIGHT_SHADER__
#define __DIRECT_LIGHT_SHADER__

#include "environment/scene/Scene.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/light/Light.h"
#include "environment/material/povray/PovRayMaterial.h"
#include "java/util/ArrayList.h"
#include "render/bakedScene/BakedScene.h"
#include "render/shaders/TraceService.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class DirectLightShader {
public:
    static void shade(const PovRayMaterial *texture, const Vector3Dd *intersectionPoint,
        const RayWithSegments *eye, const Vector3Dd *surfaceNormal,
        const ColorRgba *surfaceColor, ColorRgba *color, double attenuation,
        const TraceService *traceService, const java::ArrayList<Light*> &lightSources,
        const BakedScene &bakedScene);

private:
    static bool rayIntersectsAabbBefore(
        const RayWithSegments &ray, const AxisAlignedBoundingBox &box, double maxT);
    static bool canUseCsgFirstHitForShadow(const BakedScene &bakedScene, int objectIndex);
    static bool traceShadowObject(
        const BakedScene &bakedScene,
        int objectIndex,
        RayWithSegments *lightSourceRay,
        java::PriorityQueue<IntersectionCandidate> *localDepthQueue,
        double lightSourceDepth,
        ColorRgba *lightColor,
        const TraceService *traceService);
};

#endif

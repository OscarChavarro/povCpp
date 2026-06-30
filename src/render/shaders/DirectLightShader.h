#ifndef __DIRECT_LIGHT_SHADER__
#define __DIRECT_LIGHT_SHADER__

#include "environment/scene/Scene.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/light/Light.h"
#include "environment/material/povray/PovRayMaterial.h"
#include "java/util/ArrayList.h"
#include "render/shaders/TraceService.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class DirectLightShader {
public:
    static void shade(const PovRayMaterial *texture, const Vector3Dd *intersectionPoint,
        const RayWithSegments *eye, const Vector3Dd *surfaceNormal,
        const ColorRgba *surfaceColor, ColorRgba *color, double attenuation,
        const TraceService *traceService, const java::ArrayList<Light*> &lightSources,
        const java::ArrayList<Scene::CompiledTracingObject> &boundedTracingObjects,
        const java::ArrayList<Scene::CompiledTracingObject> &unboundedTracingObjects,
        const java::ArrayList<Scene::BakedComposite> &bakedComposites,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const java::ArrayList<Scene::BakedSimpleBody> &bakedSimpleBodies);
};

#endif

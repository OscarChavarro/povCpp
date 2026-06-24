#include "java/util/PriorityQueue.txx"
#include "environment/material/RendererConfiguration.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/PovRayHit.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "render/shaders/AmbientLightShader.h"
#include "render/shaders/BumpNormalShader.h"
#include "render/shaders/DirectLightShader.h"
#include "render/shaders/LocalSurfaceShader.h"
#include "render/shaders/MirrorReflectionShader.h"
#include "render/shaders/TraceService.h"

void
LocalSurfaceShader::shade(const RayWithSegments *ray, PovRayMaterial *texture,
    IntersectionCandidate *rayIntersection, ColorRgba *surfaceColor,
    const ColorRgba *filterColor, ColorRgba *color,
    const TraceService *traceService, const Light *lightSources,
    const java::ArrayList<BoundedGeometry*> &objects,
    int &traceLevel, TextureUtils *textureUtils)
{
    PovRayHit hit = PovRayHit::fromCandidate(*rayIntersection);
    Vector3Dd surfaceNormal;
    double normalDirection;
    double attenuation;
    ColorRgba emittedColor(0.0, 0.0, 0.0, 0.0);

    // This variable keeps track of how much color comes from the surface
    // of the object and how much is transmited through

    emittedColor.setR(0.0); emittedColor.setG(0.0); emittedColor.setB(0.0); emittedColor.setA(0);

    if (texture == nullptr) {
        texture = static_cast<PovRayMaterial *>(hit.objectTexture);
    }

    if (!ray->getConfig()->withSurfaceLighting()) {
        surfaceColor->setA(0.0);
        color->setR(color->getR() + surfaceColor->getR() * filterColor->getA());
        color->setG(color->getG() + surfaceColor->getG() * filterColor->getA());
        color->setB(color->getB() + surfaceColor->getB() * filterColor->getA());
        return;
    }

    surfaceNormal = hit.n;

    if (ray->getConfig()->withBumpMapping()) {
        BumpNormalShader::shade(
            &surfaceNormal, texture, &hit.p, &surfaceNormal,
            textureUtils);
    }

    // If the surface normal points away, flip its direction
    normalDirection = surfaceNormal.dotProduct(ray->getDirection());
    if (normalDirection > 0.0) {
        surfaceNormal = surfaceNormal.multiply(-1.0);
    }

    attenuation = filterColor->getA() * (1.0 - surfaceColor->getA());

    AmbientLightShader::shade(texture, surfaceColor, &emittedColor, attenuation);
    DirectLightShader::shade(texture, &hit.p, ray, &surfaceNormal,
        surfaceColor, &emittedColor, attenuation, traceService,
        lightSources, objects);
    color->setR(color->getR() + emittedColor.getR());
    color->setG(color->getG() + emittedColor.getG());
    color->setB(color->getB() + emittedColor.getB());
    if (ray->getConfig()->withReflection()) {
        MirrorReflectionShader::shade(
            texture, &hit.p, ray, &surfaceNormal, color,
            traceService, traceLevel);
    }
}

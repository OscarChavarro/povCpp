#include "java/util/PriorityQueue.txx"
#include "environment/material/RendererConfiguration.h"
#include "environment/geometry/element/Intersection.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/geometry/SimpleBody.h"
#include "render/shaders/AmbientLightShader.h"
#include "render/shaders/BumpNormalShader.h"
#include "render/shaders/DirectLightShader.h"
#include "render/shaders/LocalSurfaceShader.h"
#include "render/shaders/MirrorReflectionShader.h"
#include "render/shaders/TraceService.h"

void
LocalSurfaceShader::shade(const RayWithSegments *ray, PovRayMaterial *texture,
    Intersection *rayIntersection, ColorRgba *surfaceColor,
    const ColorRgba *filterColor, ColorRgba *color,
    const TraceService *traceService, const Light *lightSources,
    const java::ArrayList<BoundedGeometry*> &objects,
    int &traceLevel, TextureUtils *textureUtils)
{
    Vector3Dd surfaceNormal;
    double normalDirection;
    double attenuation;
    ColorRgba emittedColor(0.0, 0.0, 0.0, 0.0);

    // This variable keeps track of how much color comes from the surface
    // of the object and how much is transmited through

    emittedColor.setR(0.0); emittedColor.setG(0.0); emittedColor.setB(0.0); emittedColor.setA(0);

    if (texture == nullptr) {
        texture = static_cast<PovRayMaterial *>(rayIntersection->getBoundedGeometry()->getObjectTexture());
    }

    if (ray->getConfig()->getQuality() <= 1) {
        surfaceColor->setA(0.0);
        color->setR(color->getR() + surfaceColor->getR() * filterColor->getA());
        color->setG(color->getG() + surfaceColor->getG() * filterColor->getA());
        color->setB(color->getB() + surfaceColor->getB() * filterColor->getA());
        return;
    }

    rayIntersection->getOwnerSimpleBody()->normal(
        &surfaceNormal, &rayIntersection->getPoint(), ray->getConfig());

    if (ray->getConfig()->getQuality() >= 8) {
        BumpNormalShader::shade(
            &surfaceNormal, texture, &rayIntersection->getPoint(), &surfaceNormal,
            textureUtils);
    }

    // If the surface normal points away, flip its direction
    normalDirection = surfaceNormal.dotProduct(ray->getDirection());
    if (normalDirection > 0.0) {
        surfaceNormal = surfaceNormal.multiply(-1.0);
    }

    attenuation = filterColor->getA() * (1.0 - surfaceColor->getA());

    AmbientLightShader::shade(texture, surfaceColor, &emittedColor, attenuation);
    DirectLightShader::shade(texture, &rayIntersection->getPoint(), ray, &surfaceNormal,
        surfaceColor, &emittedColor, attenuation, traceService,
        lightSources, objects);
    color->setR(color->getR() + emittedColor.getR());
    color->setG(color->getG() + emittedColor.getG());
    color->setB(color->getB() + emittedColor.getB());
    if (ray->getConfig()->getQuality() >= 8) {
        MirrorReflectionShader::shade(
            texture, &rayIntersection->getPoint(), ray, &surfaceNormal, color,
            traceService, traceLevel);
    }
}

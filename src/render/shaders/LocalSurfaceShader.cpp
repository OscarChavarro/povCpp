#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/Intersection.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include "environment/material/RendererConfiguration.h"
#include "render/shaders/AmbientLightShader.h"
#include "render/shaders/BumpNormalShader.h"
#include "render/shaders/DirectLightShader.h"
#include "render/shaders/LocalSurfaceShader.h"
#include "render/shaders/MirrorReflectionShader.h"
#include "render/shaders/TraceService.h"

void
LocalSurfaceShader::shade(const RayWithSegments *ray, Material *texture,
    Intersection *rayIntersection, ColorRgba *surfaceColor,
    const ColorRgba *filterColor, ColorRgba *color,
    const TraceService *traceService, const Light *lightSources,
    SimpleBody *objects, int &traceLevel)
{
    Vector3Dd surfaceNormal;
    double normalDirection;
    double attenuation;
    ColorRgba emittedColor;

    // This variable keeps track of how much color comes from the surface
    // of the object and how much is transmited through

    emittedColor.setR(0.0); emittedColor.setG(0.0); emittedColor.setB(0.0); emittedColor.setA(0);

    if (texture == nullptr) {
        texture = rayIntersection->Object->objectTexture;
    }

    if (RenderingConfiguration::global().quality <= 1) {
        surfaceColor->setA(0.0);

        color->setR(color->getR() + surfaceColor->getR() * filterColor->getA());
        color->setG(color->getG() + surfaceColor->getG() * filterColor->getA());
        color->setB(color->getB() + surfaceColor->getB() * filterColor->getA());
        return;
    }

    GeometryOperations::normal(&surfaceNormal,
        (SimpleBody *)rayIntersection->Shape, &rayIntersection->Point);

    if (RenderingConfiguration::global().quality >= 8) {
        BumpNormalShader::shade(
            &surfaceNormal, texture, &rayIntersection->Point, &surfaceNormal);
    }

    // If the surface normal points away, flip its direction
    normalDirection = surfaceNormal.dotProduct(ray->direction);
    if (normalDirection > 0.0) {
        surfaceNormal = surfaceNormal.multiply(-1.0);
    }

    attenuation = filterColor->getA() * (1.0 - surfaceColor->getA());

    AmbientLightShader::shade(texture, surfaceColor, &emittedColor, attenuation);
    DirectLightShader::shade(texture, &rayIntersection->Point, ray, &surfaceNormal,
        surfaceColor, &emittedColor, attenuation, traceService,
        lightSources, objects);
    color->setR(color->getR() + emittedColor.getR());
    color->setG(color->getG() + emittedColor.getG());
    color->setB(color->getB() + emittedColor.getB());
    if (RenderingConfiguration::global().quality >= 8) {
        MirrorReflectionShader::shade(
            texture, &rayIntersection->Point, ray, &surfaceNormal, color,
            traceService, traceLevel);
    }
}
#include "common/dataStructures/PriorityQueue.txx"

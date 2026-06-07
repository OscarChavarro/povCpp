#include "render/shaders/LocalSurfaceShader.h"
#include "render/shaders/TraceService.h"
#include "common/color/Color.h"
#include "environment/geometry/Intersection.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/material/RendererConfiguration.h"
#include "render/shaders/AmbientLightShader.h"
#include "render/shaders/BumpNormalShader.h"
#include "render/shaders/DirectLightShader.h"
#include "render/shaders/MirrorReflectionShader.h"

void
LocalSurfaceShader::shade(RayWithSegments *ray, Texture *texture,
    Intersection *rayIntersection, RGBAColor *surfaceColor,
    RGBAColor *filterColor, RGBAColor *color,
    const TraceService *traceService, Light *lightSources,
    SimpleBody *objects, int &traceLevel)
{
    Vector3Dd surfaceNormal;
    double normalDirection;
    double attenuation;
    RGBAColor emittedColor;

    /* This variable keeps track of how much color comes from the surface
of the object and how much is transmited through. */

    Color::makeColor(&emittedColor, 0.0, 0.0, 0.0);

    if (texture == nullptr) {
        texture = rayIntersection->Object->objectTexture;
    }

    if (RenderingConfiguration::global().quality <= 1) {
        surfaceColor->Alpha = 0.0;

        color->Red += surfaceColor->Red * filterColor->Alpha;
        color->Green += surfaceColor->Green * filterColor->Alpha;
        color->Blue += surfaceColor->Blue * filterColor->Alpha;
        return;
    }

    GeometryOperations::normal(&surfaceNormal,
        (SimpleBody *)rayIntersection->Shape, &rayIntersection->Point);

    if (RenderingConfiguration::global().quality >= 8) {
        BumpNormalShader::shade(
            &surfaceNormal, texture, &rayIntersection->Point, &surfaceNormal);
    }

    /* If the surface normal points away, flip its direction. */
    normalDirection = surfaceNormal.dotProduct(ray->direction);
    if (normalDirection > 0.0) {
        surfaceNormal.scale(-1.0);
    }

    attenuation = filterColor->Alpha * (1.0 - surfaceColor->Alpha);

    AmbientLightShader::shade(texture, surfaceColor, &emittedColor, attenuation);
    DirectLightShader::shade(texture, &rayIntersection->Point, ray, &surfaceNormal,
        surfaceColor, &emittedColor, attenuation, traceService,
        lightSources, objects);
    color->Red += emittedColor.Red;
    color->Green += emittedColor.Green;
    color->Blue += emittedColor.Blue;
    if (RenderingConfiguration::global().quality >= 8) {
        MirrorReflectionShader::shade(
            texture, &rayIntersection->Point, ray, &surfaceNormal, color,
            traceService, traceLevel);
    }
}

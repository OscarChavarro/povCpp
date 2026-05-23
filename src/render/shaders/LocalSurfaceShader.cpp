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
    Intersection *rayIntersection, RGBAColor *surfaceColour,
    RGBAColor *filterColour, RGBAColor *colour,
    const TraceService *traceService)
{
    Vector3Dd surfaceNormal;
    double normalDirection, attenuation;
    RGBAColor emittedColour;

    /* This variable keeps track of how much colour comes from the surface
of the object and how much is transmited through. */

    Color::makeColor(&emittedColour, 0.0, 0.0, 0.0);

    if (texture == nullptr) {
        texture = rayIntersection->Object->Object_Texture;
    }

    if (globalRenderingConfiguration.quality <= 1) {
        surfaceColour->Alpha = 0.0;

        colour->Red += surfaceColour->Red * filterColour->Alpha;
        colour->Green += surfaceColour->Green * filterColour->Alpha;
        colour->Blue += surfaceColour->Blue * filterColour->Alpha;
        return;
    }

    GeometryOperations::normal(&surfaceNormal,
        (SimpleBody *)rayIntersection->Shape, &rayIntersection->Point);

    if (globalRenderingConfiguration.quality >= 8) {
        BumpNormalShader::shade(
            &surfaceNormal, texture, &rayIntersection->Point, &surfaceNormal);
    }

    /* If the surface normal points away, flip its direction. */
    normalDirection = surfaceNormal.dotProduct(ray->direction);
    if (normalDirection > 0.0) {
        surfaceNormal.scale(-1.0);
    }

    attenuation = filterColour->Alpha * (1.0 - surfaceColour->Alpha);

    AmbientLightShader::shade(texture, surfaceColour, &emittedColour, attenuation);
    DirectLightShader::shade(texture, &rayIntersection->Point, ray, &surfaceNormal,
        surfaceColour, &emittedColour, attenuation, traceService);
    colour->Red += emittedColour.Red;
    colour->Green += emittedColour.Green;
    colour->Blue += emittedColour.Blue;
    if (globalRenderingConfiguration.quality >= 8) {
        MirrorReflectionShader::shade(texture, &rayIntersection->Point, ray, &surfaceNormal, colour, traceService);
    }
}

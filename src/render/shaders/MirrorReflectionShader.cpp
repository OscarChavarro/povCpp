#include "render/shaders/MirrorReflectionShader.h"
#include "render/shaders/TraceService.h"
#include "common/Statistics.h"
#include "common/color/Color.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryConstants.h"
#include "environment/geometry/elements/RayWithSegments.h"

extern int traceLevel;

void
MirrorReflectionShader::shade(Texture *texture, Vector3Dd *intersectionPoint,
    RayWithSegments *ray, Vector3Dd *surfaceNormal, RGBAColor *color,
    const TraceService *traceService)
{
    RayWithSegments newRay;
    RGBAColor tempColor;
    Vector3Dd localNormal;
    Vector3Dd normalProjection;
    Vector3Dd surfaceOffset;
    double normalComponent;

    if (texture->objectReflection != 0.0) {
        globalStatistics.reflectedRaysTraced++;
        normalComponent = ray->direction.dotProduct(*surfaceNormal);
        if (normalComponent < 0.0) {
            localNormal = *surfaceNormal;
            normalComponent *= -1.0;
        } else {
            VectorOps::vScale(localNormal, *surfaceNormal, -1.0);
        }

        VectorOps::vScale(normalProjection, localNormal, normalComponent);
        normalProjection.scale(2.0);
        VectorOps::vAdd(newRay.direction, ray->direction, normalProjection);
        newRay.position = *intersectionPoint;

        /* ARE 08/25/91 */

        VectorOps::vScale(
            surfaceOffset, newRay.direction, 2.0 * Small_Tolerance);
        newRay.position.add(surfaceOffset);

        newRay.copyContainersFrom(ray);
        traceLevel++;
        Color::makeColor(&tempColor, 0.0, 0.0, 0.0);
        newRay.quadricConstantsCached = FALSE;
        traceService->trace(&newRay, &tempColor);
        traceLevel--;

        color->Red += tempColor.Red * texture->objectReflection;
        color->Green += tempColor.Green * texture->objectReflection;
        color->Blue += tempColor.Blue * texture->objectReflection;
    }
}

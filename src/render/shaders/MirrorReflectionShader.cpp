#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/GeometryConstants.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include "render/shaders/MirrorReflectionShader.h"
#include "render/shaders/TraceService.h"


void
MirrorReflectionShader::shade(Material *texture, Vector3Dd *intersectionPoint,
    RayWithSegments *ray, Vector3Dd *surfaceNormal, ColorRgba *color,
    const TraceService *traceService, int &traceLevel)
{
    RayWithSegments newRay;
    ColorRgba tempColor;
    Vector3Dd localNormal;
    Vector3Dd normalProjection;
    Vector3Dd surfaceOffset;
    double normalComponent;

    if (texture->objectReflection != 0.0) {
        Statistics::global().reflectedRaysTraced++;
        normalComponent = ray->direction.dotProduct(*surfaceNormal);
        if (normalComponent < 0.0) {
            localNormal = *surfaceNormal;
            normalComponent *= -1.0;
        } else {
            localNormal = (*surfaceNormal).multiply(-1.0);
        }

        normalProjection = localNormal.multiply(normalComponent);
        normalProjection = normalProjection.multiply(2.0);
        newRay.direction = ray->direction.add(normalProjection);
        newRay.position = *intersectionPoint;

        // ARE 08/25/91

        surfaceOffset = newRay.direction.multiply(2.0 * GeometryConstants::Small_Tolerance);
        newRay.position = newRay.position.add(surfaceOffset);

        newRay.copyContainersFrom(ray);
        traceLevel++;
        tempColor.setR(0.0); tempColor.setG(0.0); tempColor.setB(0.0); tempColor.setA(0);
        newRay.quadricConstantsCached = false;
        traceService->trace(&newRay, &tempColor);
        traceLevel--;

        color->setR(color->getR() + tempColor.getR() * texture->objectReflection);
        color->setG(color->getG() + tempColor.getG() * texture->objectReflection);
        color->setB(color->getB() + tempColor.getB() * texture->objectReflection);
    }
}

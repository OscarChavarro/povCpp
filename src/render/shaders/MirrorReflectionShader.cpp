#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "common/Config.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/element/RayWithTracingState.h"
#include "render/shaders/MirrorReflectionShader.h"
#include "render/shaders/TraceService.h"

void
MirrorReflectionShader::shade(const PovRayMaterial *texture, const Vector3Dd *intersectionPoint,
    const RayWithTracingState *ray, const Vector3Dd *surfaceNormal,
    ColorRgba *color, const TraceService *traceService, int &traceLevel)
{
    RayWithTracingState newRay;
    ColorRgba tempColor(0.0, 0.0, 0.0, 0.0);
    Vector3Dd localNormal;
    Vector3Dd normalProjection;
    Vector3Dd surfaceOffset;
    double normalComponent;

    if (texture->getObjectReflection() != 0.0) {
        Statistics &stats = *ray->getStatistics();
        stats.incrementReflectedRaysTraced();
        normalComponent = ray->getDirection().dotProduct(*surfaceNormal);
        if (normalComponent < 0.0) {
            localNormal = *surfaceNormal;
            normalComponent *= -1.0;
        } else {
            localNormal = (*surfaceNormal).multiply(-1.0);
        }

        normalProjection = localNormal.multiply(normalComponent);
        normalProjection = normalProjection.multiply(2.0);
        newRay.setDirection(ray->getDirection().add(normalProjection));
        newRay.setOrigin(*intersectionPoint);

        surfaceOffset = newRay.getDirection().multiply(2.0 * Config::SMALL_TOLERANCE);
        newRay.setOrigin(newRay.getOrigin().add(surfaceOffset));

        newRay.copyContainersFrom(ray);
        newRay.setStatistics(ray->getStatistics());
        newRay.setConfig(ray->getConfig());
        newRay.setIntersectionQueuePool(ray->getIntersectionQueuePool());
        traceLevel++;
        tempColor.setR(0.0); tempColor.setG(0.0); tempColor.setB(0.0); tempColor.setA(0);
        newRay.setQuadricConstantsCached(false);
        traceService->trace(&newRay, &tempColor);
        traceLevel--;

        color->setR(color->getR() + tempColor.getR() * texture->getObjectReflection());
        color->setG(color->getG() + tempColor.getG() * texture->getObjectReflection());
        color->setB(color->getB() + tempColor.getB() * texture->getObjectReflection());
    }
}

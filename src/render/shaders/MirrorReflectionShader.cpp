#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/GeometryConstants.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "render/shaders/MirrorReflectionShader.h"
#include "render/shaders/TraceService.h"

void
MirrorReflectionShader::shade(const PovrayMaterial *texture, const Vector3Dd *intersectionPoint,
    const RayWithSegments *ray, const Vector3Dd *surfaceNormal,
    const TraceService *traceService)
{
    RayWithSegments newRay;
    ColorRgba multiplier(0.0, 0.0, 0.0, 0.0);
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

        surfaceOffset = newRay.getDirection().multiply(2.0 * GeometryConstants::Small_Tolerance);
        newRay.setOrigin(newRay.getOrigin().add(surfaceOffset));

        newRay.copyContainersFrom(ray);
        newRay.setStatistics(ray->getStatistics());
        newRay.setConfig(ray->getConfig());
        newRay.setIntersectionQueuePool(ray->getIntersectionQueuePool());
        newRay.setQuadricConstantsCached(false);
        multiplier.setR(texture->getObjectReflection());
        multiplier.setG(texture->getObjectReflection());
        multiplier.setB(texture->getObjectReflection());
        traceService->trace(&newRay, &multiplier);
    }
}

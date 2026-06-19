#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/GeometryConstants.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "render/shaders/TraceService.h"
#include "render/shaders/TransmissionRefractionShader.h"

void
TransmissionRefractionShader::shade(PovrayMaterial *texture, const Vector3Dd *intersectionPoint,
    const RayWithSegments *ray, const Vector3Dd *surfaceNormal, ColorRgba *color,
    const TraceService *traceService, double atmosphereIor, int &traceLevel)
{
    RayWithSegments newRay;
    ColorRgba tempColor(0.0, 0.0, 0.0, 0.0);
    Vector3Dd localNormal;
    Vector3Dd rayDirection;
    double normalComponent;
    double tempIor;
    double temp;
    double ior;

    Statistics &stats = *ray->getStatistics();

    if (surfaceNormal == nullptr) {
        newRay.setOriginAndDirection(*intersectionPoint, ray->getDirection());

        newRay.copyContainersFrom(ray);
        newRay.setStatistics(ray->getStatistics());
        newRay.setConfig(ray->getConfig());
        newRay.setIntersectionQueuePool(ray->getIntersectionQueuePool());
        traceLevel++;
        stats.incrementTransmittedRaysTraced();
        tempColor.setR(0.0); tempColor.setG(0.0); tempColor.setB(0.0); tempColor.setA(0);
        newRay.setQuadricConstantsCached(false);
        traceService->trace(&newRay, &tempColor);
        traceLevel--;
        color->setR(color->getR() + tempColor.getR());
        color->setG(color->getG() + tempColor.getG());
        color->setB(color->getB() + tempColor.getB());
    } else {
        stats.incrementRefractedRaysTraced();
        normalComponent = ray->getDirection().dotProduct(*surfaceNormal);
        if (normalComponent <= 0.0) {
            localNormal = Vector3Dd(
                surfaceNormal->x(), surfaceNormal->y(), surfaceNormal->z());
            normalComponent *= -1.0;
        } else {
            localNormal = (*surfaceNormal).multiply(-1.0);
        }

        newRay.copyContainersFrom(ray);
        newRay.setStatistics(ray->getStatistics());
        newRay.setConfig(ray->getConfig());
        newRay.setIntersectionQueuePool(ray->getIntersectionQueuePool());

        if (ray->getContainingIndex() == -1) {
            // The ray is entering from the atmosphere
            newRay.enterContainingMedium(texture);
            ior = atmosphereIor /
                  (texture->getObjectIndexOfRefraction());
        } else {
            // The ray is currently inside an object
            if (newRay.getContainingTextureAt(newRay.getContainingIndex()) == texture)
            // if (inside)
            {
                // The ray is leaving the current object
                newRay.exitContainingMedium();
                if (newRay.getContainingIndex() == -1) {
                    // The ray is leaving into the atmosphere
                    tempIor = atmosphereIor;
                } else {
                    // The ray is leaving into another object
                    tempIor = newRay.getContainingIORAt(newRay.getContainingIndex());
                }

                ior = (texture->getObjectIndexOfRefraction()) / tempIor;
            } else {
                // The ray is entering a new object
                tempIor = newRay.getContainingIORAt(newRay.getContainingIndex());
                newRay.enterContainingMedium(texture);

                ior = tempIor / (texture->getObjectIndexOfRefraction());
            }
        }

        temp = 1.0 + ior * ior * (normalComponent * normalComponent - 1.0);
        if (temp < 0.0) {
            // Total internal reflection - not yet implemented.
            // reflect (texture, intersectionPoint, ray, surfaceNormal, color);
            return;
        }

        temp = ior * normalComponent - java::Math::sqrt(temp);
        localNormal = localNormal.multiply(temp);
        rayDirection = ray->getDirection().multiply(ior);
        newRay.setDirection(localNormal.add(rayDirection));
        newRay.setDirection(newRay.getDirection().normalizedFast());

        newRay.setOrigin(*intersectionPoint);
        traceLevel++;
        tempColor.setR(0.0); tempColor.setG(0.0); tempColor.setB(0.0); tempColor.setA(0);
        newRay.setQuadricConstantsCached(false);

        traceService->trace(&newRay, &tempColor);
        traceLevel--;

        color->setR(color->getR() + tempColor.getR() * texture->getObjectRefraction());
        color->setG(color->getG() + tempColor.getG() * texture->getObjectRefraction());
        color->setB(color->getB() + tempColor.getB() * texture->getObjectRefraction());
    }
}

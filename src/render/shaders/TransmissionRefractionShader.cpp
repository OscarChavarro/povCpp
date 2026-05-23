#include "render/RenderEngine.h"
#include "render/shaders/TransmissionRefractionShader.h"
#include "render/shaders/TraceService.h"
#include "common/Statistics.h"
#include "common/color/Color.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryConstants.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include "environment/scene/SceneFrame.h"


void
TransmissionRefractionShader::shade(Texture *texture, Vector3Dd *intersectionPoint,
    RayWithSegments *ray, Vector3Dd *surfaceNormal, RGBAColor *color,
    const TraceService *traceService)
{
    RayWithSegments newRay;
    RGBAColor tempColor;
    Vector3Dd localNormal;
    Vector3Dd rayDirection;
    double normalComponent;
    double tempIor;
    double temp;
    double ior;

    if (surfaceNormal == nullptr) {
        newRay.position = *intersectionPoint;
        newRay.direction = ray->direction;

        newRay.copyContainersFrom(ray);
        RenderEngine::traceLevel()++;
        globalStatistics.transmittedRaysTraced++;
        Color::makeColor(&tempColor, 0.0, 0.0, 0.0);
        newRay.quadricConstantsCached = FALSE;
        traceService->trace(&newRay, &tempColor);
        RenderEngine::traceLevel()--;
        (color->Red) += tempColor.Red;
        (color->Green) += tempColor.Green;
        (color->Blue) += tempColor.Blue;
    } else {
        globalStatistics.refractedRaysTraced++;
        normalComponent = ray->direction.dotProduct(*surfaceNormal);
        if (normalComponent <= 0.0) {
            localNormal.x = surfaceNormal->x;
            localNormal.y = surfaceNormal->y;
            localNormal.z = surfaceNormal->z;
            normalComponent *= -1.0;
        } else {
            VectorOps::vScale(localNormal, *surfaceNormal, -1.0);
        }

        newRay.copyContainersFrom(ray);

        if (ray->containingIndex == -1) {
            /* The ray is entering from the atmosphere */
            newRay.enterContainingMedium(texture);
            ior = (RenderEngine::renderFrame().atmosphereIor) /
                  (texture->objectIndexOfRefraction);
        } else {
            /* The ray is currently inside an object */
            if (newRay.containingTextures[newRay.containingIndex] == texture)
            /*            if (inside) */
            {
                /* The ray is leaving the current object */
                newRay.exitContainingMedium();
                if (newRay.containingIndex == -1) {
                    /* The ray is leaving into the atmosphere */
                    tempIor = RenderEngine::renderFrame().atmosphereIor;
                } else {
                    /* The ray is leaving into another object */
                    tempIor = newRay.containingIORs[newRay.containingIndex];
                }

                ior = (texture->objectIndexOfRefraction) / tempIor;
            } else {
                /* The ray is entering a new object */
                tempIor = newRay.containingIORs[newRay.containingIndex];
                newRay.enterContainingMedium(texture);

                ior = tempIor / (texture->objectIndexOfRefraction);
            }
        }

        temp = 1.0 + ior * ior * (normalComponent * normalComponent - 1.0);
        if (temp < 0.0) {
            /* Total internal reflection - not yet implemented.
    reflect (texture, intersectionPoint, ray, surfaceNormal, color);
    */
            return;
        }

        temp = ior * normalComponent - sqrt(temp);
        localNormal.scale(temp);
        VectorOps::vScale(rayDirection, ray->direction, ior);
        VectorOps::vAdd(newRay.direction, localNormal, rayDirection);
        newRay.direction.normalize();

        newRay.position = *intersectionPoint;
        RenderEngine::traceLevel()++;
        Color::makeColor(&tempColor, 0.0, 0.0, 0.0);
        newRay.quadricConstantsCached = FALSE;

        traceService->trace(&newRay, &tempColor);
        RenderEngine::traceLevel()--;

        (color->Red) += (tempColor.Red) * (texture->objectRefraction);
        (color->Green) += (tempColor.Green) * (texture->objectRefraction);
        (color->Blue) += (tempColor.Blue) * (texture->objectRefraction);
    }
}

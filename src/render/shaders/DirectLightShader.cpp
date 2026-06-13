#include "common/dataStructures/PriorityQueue.txx"
#include "common/statistics/Statistics.h"
#include "common/dataStructures/PriorityQueue.h"
#include "environment/geometry/GeometryConstants.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/Intersection.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include "environment/material/RendererConfiguration.h"
#include "environment/light/Light.h"
#include "render/shaders/BlinnPhongSpecularShader.h"
#include "render/shaders/DirectLightShader.h"
#include "render/shaders/LambertShader.h"
#include "render/shaders/LightSamplerShader.h"
#include "render/shaders/PhongSpecularShader.h"
#include "render/shaders/ShadowShader.h"
#include "render/shaders/TraceService.h"


static constexpr double SHADOW_TOLERANCE = 0.05;

void
DirectLightShader::shade(const Material *texture, const Vector3Dd *intersectionPoint,
    const RayWithSegments *eye, const Vector3Dd *surfaceNormal, const ColorRgba *surfaceColor,
    ColorRgba *color, double attenuation, const TraceService *traceService,
    const Light *lightSources, SimpleBody *objects)
{
    double lightSourceDepth;
    RayWithSegments lightSourceRay;
    const Light *lightSource;
    SimpleBody *blockingObject;
    bool intersectionFound;
    Intersection localIntersection;
    Vector3Dd rEye;
    ColorRgba lightColor;
    PriorityQueueNode *localQueue;

    rEye = Vector3Dd(0, 0, 0);

    if ((texture->objectDiffuse == 0.0) && (texture->objectSpecular == 0.0) &&
        (texture->objectPhong == 0.0)) {
        return;
    }

    if (texture->objectSpecular != 0.0) {
        rEye = Vector3Dd(
            -eye->direction.x(), -eye->direction.y(), -eye->direction.z());
    }

    localQueue = IntersectionPriorityQueuePool::pqPop(128);
    lightSourceRay.isShadowRay = true;
    lightSourceRay.isPrimaryRay = false;

    for (lightSource = lightSources; lightSource != nullptr;
        lightSource = lightSource->Next_Light_Source) {
        intersectionFound = false;

        LightSamplerShader::sample(lightSource, &lightSourceDepth, &lightSourceRay,
            intersectionPoint, &lightColor);

        // What objects does this ray intersect?
        if (RenderingConfiguration::global().quality > 3) {
            for (blockingObject = objects;
                blockingObject != nullptr;
                blockingObject = blockingObject->nextObject) {

                Statistics::global().shadowRayTests++;
                GeometryOperations::allIntersections(
                    blockingObject, &lightSourceRay, localQueue);
                while (localQueue->size() > 0) {
                    localIntersection = localQueue->poll();

                    if ((localIntersection.Depth <
                            lightSourceDepth - GeometryConstants::Small_Tolerance) &&
                        (localIntersection.Depth > SHADOW_TOLERANCE)) {

                        // Does the object not cast a shadow?
                        if (!localIntersection.Object->noShadowFlag) {
                            if (ShadowShader::shade(&localIntersection, &lightColor,
                                    localQueue, traceService)) {
                                intersectionFound = true;
                                break;
                            }
                        }
                    }
                }
                if (intersectionFound) {
                    break;
                }
            }
        }

        // If light source was not blocked by any intervening object, then
        // calculate it's contribution to the object's overall illumination

        if (!intersectionFound) {
            if (texture->objectPhong > 0.0) { // Phong Hilite
                PhongSpecularShader::shade(texture, &lightSourceRay, eye->direction, surfaceNormal,
                    color, &lightColor, surfaceColor);
            }

            if (texture->objectSpecular > 0.0) { // Specular Hilite
                BlinnPhongSpecularShader::shade(texture, &lightSourceRay, rEye, surfaceNormal,
                    color, &lightColor, surfaceColor);
            }

            if (texture->objectDiffuse > 0.0) { // Normal Diffuse Illum.
                LambertShader::shade(texture, &lightSourceRay, surfaceNormal, color,
                    &lightColor, surfaceColor, attenuation);
            }
        }
    }
    IntersectionPriorityQueuePool::pqPush(localQueue);
}

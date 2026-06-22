#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"
#include "common/Config.h"
#include "environment/geometry/element/IntersectionPriorityQueuePool.h"
#include "common/statistics/Statistics.h"
#include "environment/material/RendererConfiguration.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"
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
DirectLightShader::shade(const PovRayMaterial *texture, const Vector3Dd *intersectionPoint,
    const RayWithSegments *eye, const Vector3Dd *surfaceNormal, const ColorRgba *surfaceColor,
    ColorRgba *color, double attenuation, const TraceService *traceService,
    const Light *lightSources, const java::ArrayList<BoundedGeometry*> &objects)
{
    double lightSourceDepth;
    RayWithSegments lightSourceRay;
    const Light *lightSource;
    BoundedGeometry *blockingObject;
    bool intersectionFound;
    IntersectionCandidate localIntersection;
    Vector3Dd rEye;
    ColorRgba lightColor(0.0, 0.0, 0.0, 0.0);
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue;

    rEye = Vector3Dd(0, 0, 0);

    if ((texture->getObjectDiffuse() == 0.0) && (texture->getObjectSpecular() == 0.0) &&
        (texture->getObjectPhong() == 0.0)) {
        return;
    }

    if (texture->getObjectSpecular() != 0.0) {
        rEye = Vector3Dd(
            -eye->getDirection().x(), -eye->getDirection().y(), -eye->getDirection().z());
    }

    localDepthQueue = eye->getIntersectionQueuePool()->pop(128);
    lightSourceRay.setShadowRay(true);
    lightSourceRay.setPrimaryRay(false);
    lightSourceRay.setStatistics(eye->getStatistics());
    lightSourceRay.setConfig(eye->getConfig());
    lightSourceRay.setIntersectionQueuePool(eye->getIntersectionQueuePool());

    for (lightSource = lightSources; lightSource != nullptr;
        lightSource = lightSource->getNextLightSource()) {
        intersectionFound = false;

        LightSamplerShader::sample(lightSource, &lightSourceDepth, &lightSourceRay,
            intersectionPoint, &lightColor);

        // What objects does this ray intersect?
        if (eye->getConfig()->getQuality() > 3) {
            Statistics &stats = *eye->getStatistics();
            for (long int i = objects.size() - 1; i >= 0; i--) {
                blockingObject = objects[i];

                stats.incrementShadowRayTests();
                blockingObject->allIntersections(&lightSourceRay, localDepthQueue);
                while (localDepthQueue->size() > 0) {
                    localIntersection = localDepthQueue->poll();

                    if ((localIntersection.getIntersection().t <
                            lightSourceDepth - Config::SMALL_TOLERANCE) &&
                        (localIntersection.getIntersection().t > SHADOW_TOLERANCE)) {

                        // Does the object not cast a shadow?
                        if (!localIntersection.getAttributes().getNoShadowFlag()) {
                            if (ShadowShader::shade(&localIntersection, &lightColor,
                                    localDepthQueue, traceService)) {
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
            if (texture->getObjectPhong() > 0.0) { // Phong Hilite
                PhongSpecularShader::shade(texture, &lightSourceRay, eye->getDirection(), surfaceNormal,
                    color, &lightColor, surfaceColor);
            }

            if (texture->getObjectSpecular() > 0.0) { // Specular Hilite
                BlinnPhongSpecularShader::shade(texture, &lightSourceRay, rEye, surfaceNormal,
                    color, &lightColor, surfaceColor);
            }

            if (texture->getObjectDiffuse() > 0.0) { // Normal Diffuse Illum.
                LambertShader::shade(texture, &lightSourceRay, surfaceNormal, color,
                    &lightColor, surfaceColor, attenuation);
            }
        }
    }
    eye->getIntersectionQueuePool()->push(localDepthQueue);
}

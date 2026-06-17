#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"
#include "common/statistics/Statistics.h"
#include "common/dataStructures/PriorityQueuePool.txx"
#include "environment/material/RendererConfiguration.h"
#include "environment/geometry/GeometryConstants.h"
#include "environment/geometry/Intersection.h"
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
DirectLightShader::shade(const PovrayMaterial *texture, const Vector3Dd *intersectionPoint,
    const RayWithSegments *eye, const Vector3Dd *surfaceNormal, const ColorRgba *surfaceColor,
    ColorRgba *color, double attenuation, const TraceService *traceService,
    const Light *lightSources, java::ArrayList<BoundedGeometry*> &objects)
{
    double lightSourceDepth;
    RayWithSegments lightSourceRay;
    const Light *lightSource;
    BoundedGeometry *blockingObject;
    bool intersectionFound;
    Intersection localIntersection;
    Vector3Dd rEye;
    ColorRgba lightColor;
    java::PriorityQueue<Intersection> *localQueue;

    rEye = Vector3Dd(0, 0, 0);

    if ((texture->getObjectDiffuse() == 0.0) && (texture->getObjectSpecular() == 0.0) &&
        (texture->getObjectPhong() == 0.0)) {
        return;
    }

    if (texture->getObjectSpecular() != 0.0) {
        rEye = Vector3Dd(
            -eye->getDirection().x(), -eye->getDirection().y(), -eye->getDirection().z());
    }

    localQueue = PriorityQueuePool<Intersection>::pqPop(128);
    lightSourceRay.isShadowRay = true;
    lightSourceRay.isPrimaryRay = false;

    for (lightSource = lightSources; lightSource != nullptr;
        lightSource = lightSource->getNextLightSource()) {
        intersectionFound = false;

        LightSamplerShader::sample(lightSource, &lightSourceDepth, &lightSourceRay,
            intersectionPoint, &lightColor);

        // What objects does this ray intersect?
        if (RenderingConfiguration::global().getQuality() > 3) {
            for (long int i = objects.size() - 1; i >= 0; i--) {
                blockingObject = objects[i];

                Statistics::global().incrementShadowRayTests();
                blockingObject->allIntersections(&lightSourceRay, localQueue);
                while (localQueue->size() > 0) {
                    localIntersection = localQueue->poll();

                    if ((localIntersection.getDepth() <
                            lightSourceDepth - GeometryConstants::Small_Tolerance) &&
                        (localIntersection.getDepth() > SHADOW_TOLERANCE)) {

                        // Does the object not cast a shadow?
                        if (!localIntersection.getObject()->getNoShadowFlag()) {
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
    PriorityQueuePool<Intersection>::pqPush(localQueue);
}

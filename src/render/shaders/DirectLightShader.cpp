#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.txx"
#include "common/Config.h"
#include "environment/geometry/element/PriorityQueuePool.txx"
#include "common/statistics/Statistics.h"
#include "environment/material/PovRayRendererConfiguration.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithTracingState.h"
#include "environment/light/Light.h"
#include "render/shaders/BlinnPhongSpecularShader.h"
#include "render/shaders/DirectLightShader.h"
#include "render/shaders/LambertShader.h"
#include "render/shaders/LightSamplerShader.h"
#include "render/shaders/PhongSpecularShader.h"
#include "render/shaders/ShadowShader.h"
#include "render/shaders/TraceService.h"
#include "render/bakedScene/BakedTrace.h"

static constexpr double SHADOW_TOLERANCE = 0.05;

bool
DirectLightShader::rayIntersectsAabbBefore(
    const RayWithTracingState &ray, const AxisAlignedBoundingBox &box, double maxT)
{
    const Vector3Dd origin = ray.getOrigin();
    const Vector3Dd direction = ray.getDirection();
    double tMin = 0.0;
    double tMax = maxT;

    auto updateAxis = [&](double originCoord, double directionCoord,
                          double minCoord, double maxCoord) -> bool {
        if (directionCoord > -1e-12 && directionCoord < 1e-12) {
            return originCoord >= minCoord && originCoord <= maxCoord;
        }
        const double invDir = 1.0 / directionCoord;
        double nearT = (minCoord - originCoord) * invDir;
        double farT = (maxCoord - originCoord) * invDir;
        if (nearT > farT) {
            const double tmp = nearT;
            nearT = farT;
            farT = tmp;
        }
        tMin = nearT > tMin ? nearT : tMin;
        tMax = farT < tMax ? farT : tMax;
        return tMin <= tMax;
    };

    return
        updateAxis(origin.x(), direction.x(), box.min.x(), box.max.x()) &&
        updateAxis(origin.y(), direction.y(), box.min.y(), box.max.y()) &&
        updateAxis(origin.z(), direction.z(), box.min.z(), box.max.z()) &&
        tMax >= 0.0;
}

bool
DirectLightShader::canUseCsgFirstHitForShadow(const BakedScene &bakedScene, int objectIndex)
{
    const BakedScene::TraceableObject &object = bakedScene.traceableObjects[objectIndex];
    return object.kind != BakedScene::TraceKind::Composite && object.csgProgramIndex >= 0;
}

bool
DirectLightShader::traceShadowObject(
    const BakedScene &bakedScene,
    int objectIndex,
    RayWithTracingState *lightSourceRay,
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue,
    double lightSourceDepth,
    ColorRgba *lightColor,
    const TraceService *traceService)
{
    RaySharedCache &cache = traceService->getRaySharedCache();
    if (!lightSourceRay->getConfig()->withFilteredShadows() &&
        canUseCsgFirstHitForShadow(bakedScene, objectIndex)) {
        IntersectionCandidate firstHit;
        if (!BakedTrace::traceFirstHit(
                bakedScene, objectIndex, lightSourceRay, firstHit, cache)) {
            return false;
        }

        const double t = firstHit.getIntersection().t;
        if (t > SHADOW_TOLERANCE &&
            t < lightSourceDepth - Config::SMALL_TOLERANCE &&
            !firstHit.getAttributes().getNoShadowFlag()) {
            return true;
        }
    }

    BakedTrace::traceAllCrossings(
        bakedScene, objectIndex, lightSourceRay, localDepthQueue, cache);

    while (localDepthQueue->size() > 0) {
        IntersectionCandidate localIntersection = localDepthQueue->poll();
        const double t = localIntersection.getIntersection().t;
        if (t >= lightSourceDepth - Config::SMALL_TOLERANCE ||
            t <= SHADOW_TOLERANCE) {
            continue;
        }

        if (localIntersection.getAttributes().getNoShadowFlag()) {
            continue;
        }

        if (!lightSourceRay->getConfig()->withFilteredShadows()) {
            localDepthQueue->clear();
            return true;
        }

        if (ShadowShader::shade(&localIntersection, lightColor,
                localDepthQueue, traceService)) {
            return true;
        }
    }
    return false;
}

void
DirectLightShader::shade(const PovRayMaterial *texture, const Vector3Dd *intersectionPoint,
    const RayWithTracingState *eye, const Vector3Dd *surfaceNormal, const ColorRgba *surfaceColor,
    ColorRgba *color, double attenuation, const TraceService *traceService,
    const java::ArrayList<Light*> &lightSources,
    const BakedScene &bakedScene)
{
    double lightSourceDepth;
    RayWithTracingState lightSourceRay;
    bool intersectionFound;
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

    traceService->getRaySharedCache().ensureCapacity(
        (int)bakedScene.statistics.quadricViewpointSlotCount,
        (int)bakedScene.statistics.planeViewpointSlotCount);

    localDepthQueue = eye->getIntersectionQueuePool()->pop(128);
    lightSourceRay.setShadowRay(true);
    lightSourceRay.setPrimaryRay(false);
    // Shadow rays only ever need doIntersectionForAllRayCrossings()'s hitDistance (ShadowShader
    // never reads a normal); document that explicitly via the mask, mirroring
    // VITRAL's RayHit::DETAIL_NONE for the same case.
    lightSourceRay.setRequiredDetailMask(RayWithTracingState::DETAIL_NONE);
    lightSourceRay.setStatistics(eye->getStatistics());
    lightSourceRay.setConfig(eye->getConfig());
    lightSourceRay.setIntersectionQueuePool(eye->getIntersectionQueuePool());

    for (long int _li = 0; _li < lightSources.size(); _li++) {
        const Light *lightSource = lightSources[_li];
        intersectionFound = false;

        LightSamplerShader::sample(lightSource, &lightSourceDepth, &lightSourceRay,
            intersectionPoint, &lightColor);

        // What objects does this ray intersect?
        if (eye->getConfig()->withShadows()) {
            Statistics &stats = *eye->getStatistics();
            const java::ArrayList<int> &boundedShadowObjects =
                bakedScene.boundedShadowCastingObjectIndices;
            const java::ArrayList<int> &unboundedShadowObjects =
                bakedScene.unboundedShadowCastingObjectIndices;
            for (long int i = boundedShadowObjects.size() - 1; i >= 0; i--) {
                const int objectIndex = boundedShadowObjects[i];
                const BakedScene::TraceableObject &entry = bakedScene.traceableObjects[objectIndex];
                if (entry.bounded &&
                    !rayIntersectsAabbBefore(
                        lightSourceRay,
                        entry.worldBounds,
                        lightSourceDepth - Config::SMALL_TOLERANCE)) {
                    continue;
                }

                stats.incrementShadowRayTests();
                if (traceShadowObject(
                    bakedScene,
                    objectIndex,
                    &lightSourceRay,
                    localDepthQueue,
                    lightSourceDepth,
                    &lightColor,
                    traceService)) {
                    intersectionFound = true;
                    break;
                }
            }
            for (long int i = unboundedShadowObjects.size() - 1;
                !intersectionFound && i >= 0;
                i--) {
                const int objectIndex = unboundedShadowObjects[i];
                stats.incrementShadowRayTests();
                if (traceShadowObject(
                    bakedScene,
                    objectIndex,
                    &lightSourceRay,
                    localDepthQueue,
                    lightSourceDepth,
                    &lightColor,
                    traceService)) {
                    intersectionFound = true;
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

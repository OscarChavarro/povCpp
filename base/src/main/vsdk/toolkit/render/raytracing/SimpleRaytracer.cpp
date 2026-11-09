#include "java/lang/Double.h"
#include <cmath>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/VSDKFatalException.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/statistics/RaytraceStatistics.h"
#include "vsdk/toolkit/gui/feedback/ProgressMonitor.h"
#include "vsdk/toolkit/media/Image.h"
#include "vsdk/toolkit/media/NormalMap.h"
#include "vsdk/toolkit/media/RGBImageUncompressed.h"
#include "vsdk/toolkit/media/RGBPixel.h"
#include "vsdk/toolkit/media/ZBuffer.h"
#include "vsdk/toolkit/render/raytracing/DepthBufferEncoder.h"
#include "vsdk/toolkit/environment/material/RendererConfiguration.h"
#include "vsdk/toolkit/environment/material/SimpleMaterial.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/background/Background.h"
#include "vsdk/toolkit/environment/camera/Camera.h"
#include "vsdk/toolkit/environment/camera/CameraSnapshot.h"
#include "vsdk/toolkit/environment/light/Light.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/scene/SimpleSceneSnapshot.h"
#include "vsdk/toolkit/render/raytracing/RasterTileArea.h"
#include "vsdk/toolkit/render/raytracing/RasterTileGenerator.h"
#include "vsdk/toolkit/render/raytracing/SimpleRaytracer.h"
#include "vsdk/toolkit/render/shaders/Shader.h"
#include "vsdk/toolkit/render/shaders/ShaderSelector.h"
SimpleRaytracer::SimpleRaytracer() : workspace(MAX_RECURSION_LEVEL)
{
}

bool SimpleRaytracer::hasNonAmbientLights(java::ArrayList<Light*>& lights)
{ for (long int i=0;i<lights.size();i++) if (lights.get(i) && !lights.get(i)->isAmbient()) return true; return false; }

bool SimpleRaytracer::isReflective(SimpleMaterial* material)
{ return material != 0 && material->getReflectionCoefficient() > 0; }

RenderContext SimpleRaytracer::buildRenderContext(const RendererConfiguration* qualitySelection,java::ArrayList<Light*>& lights)
{
    Shader* localShader = ShaderSelector::select(qualitySelection);
    bool localLightingEnabled =
        qualitySelection->getShadingType() != RendererConfiguration::SHADING_TYPE_NOLIGHT &&
        hasNonAmbientLights(lights);
    return RenderContext(localLightingEnabled, qualitySelection->isTextureSet(), qualitySelection->isBumpMapSet(), localShader);
}

int SimpleRaytracer::buildSurfaceDetailMask(SimpleMaterial* material, Image* texture, NormalMap* normalMap, const RenderContext& renderContext)
{
    bool objectReflective = isReflective(material);
    bool needsPoint = renderContext.localLightingEnabled || objectReflective;
    bool needsNormal = needsPoint;
    bool needsUv = (renderContext.textureEnabled && texture != 0) || ((renderContext.localLightingEnabled || objectReflective) && normalMap != 0);
    bool needsTangent = (renderContext.localLightingEnabled || objectReflective) && normalMap != 0;
    int detailMask = RayHit::DETAIL_NONE;
    if (needsPoint) detailMask |= RayHit::DETAIL_POINT;
    if (needsNormal) detailMask |= RayHit::DETAIL_NORMAL;
    if (needsUv) detailMask |= RayHit::DETAIL_UV;
    if (needsTangent) detailMask |= RayHit::DETAIL_TANGENT;
    return detailMask;
}

void SimpleRaytracer::captureBodyVersions(java::ArrayList<SimpleBody*>& bodies, java::ArrayList<long long>& out)
{ for (long int i=0;i<bodies.size();i++) out.add(bodies.get(i)->getModificationVersion()); }

void SimpleRaytracer::assertSceneUnmodifiedDuringRender(java::ArrayList<long long>& expected,java::ArrayList<SimpleBody*>& bodies)
{
    if ( expected.size() != bodies.size() ) {
        Logger::reportMessage("SimpleRaytracer", Logger::ERROR, "assertSceneUnmodifiedDuringRender", "Scene bodies list changed while raytracing");
        throw VSDKFatalException("Scene bodies list changed while raytracing");
    }
    for ( long int i = 0; i < bodies.size(); i++ ) {
        if ( bodies.get(i)->getModificationVersion() != expected.get(i) ) {
            Logger::reportMessage("SimpleRaytracer", Logger::ERROR, "assertSceneUnmodifiedDuringRender", "SimpleBody modified while raytracing");
            throw VSDKFatalException("SimpleBody modified while raytracing");
        }
    }
}

Ray SimpleRaytracer::generateRay(const CameraSnapshot* c, int x, int y)
{
    double viewportXSize = c->getViewportXSize();
    double viewportYSize = c->getViewportYSize();
    double pixelCenterX = (double)x + 0.5;
    double pixelCenterY = (double)y + 0.5;
    double u = (pixelCenterX - viewportXSize/2.0) / viewportXSize;
    double v = ((viewportYSize - pixelCenterY) - viewportYSize/2.0) / viewportYSize;

    if ( c->getProjectionMode() == Camera::PROJECTION_MODE_ORTHOGONAL ) {
        Vector3Dd left = c->getLeft(); Vector3Dd up = c->getUp(); Vector3Dd front = c->getFront(); Vector3Dd eye = c->getEyePosition();
        double fovFactor = viewportXSize/viewportYSize;
        double duScale = (-fovFactor) * (2*u/c->getOrthogonalZoom());
        double dvScale = 2*v/c->getOrthogonalZoom();
        Vector3Dd origin(eye.x()+left.x()*duScale+up.x()*dvScale, eye.y()+left.y()*duScale+up.y()*dvScale, eye.z()+left.z()*duScale+up.z()*dvScale);
        return Ray(origin, front);
    }

    Vector3Dd rw = c->getRightWithScale();
    Vector3Dd uw = c->getUpWithScale();
    Vector3Dd dir = c->getDir();
    Vector3Dd direction(rw.x()*u + uw.x()*v + dir.x(), rw.y()*u + uw.y()*v + dir.y(), rw.z()*u + uw.z()*v + dir.z());
    return Ray(c->getEyePosition(), direction);
}

void SimpleRaytracer::prepareSurfaceHit(SimpleBody* nearestObject, const SceneObjectRenderData& objectData, const Ray& hitRay, RayHit* outHit)
{
    outHit->reset(objectData.detailMask);
    outHit->setRay(hitRay);
    if ( objectData.detailMask != RayHit::DETAIL_NONE ) {
        nearestObject->doExtraInformation(hitRay, hitRay.getT(), outHit);
        outHit->setRay(hitRay);
    }
    if ( !outHit->needsTextureCoordinates() ) outHit->texture = 0;
    else if ( outHit->texture == 0 ) outHit->texture = objectData.texture;
    if ( !outHit->needsTextureCoordinates() || !outHit->needsNormal() || !outHit->needsTangent() ) outHit->normalMap = 0;
    else if ( outHit->normalMap == 0 ) outHit->normalMap = objectData.normalMap;
}

SimpleMaterial* SimpleRaytracer::resolveMaterial(RayHit* hit, const SceneObjectRenderData& objectData)
{ return hit->material != 0 ? hit->material : objectData.material; }

ColorRgb SimpleRaytracer::evaluateIlluminationModel(RayHit* info,double viewX,double viewY,double viewZ,java::ArrayList<Light*>& lights,java::ArrayList<SimpleBody*>& objects,const SceneRenderCache& cache,Background* background,SimpleMaterial* material,RenderContext& renderContext,int recursions,int recursionLevel)
{
    Shader::LocalShadingResult localShading = renderContext.localShader->shadeLocal(info, viewX, viewY, viewZ, lights, objects, material, &workspace);
    Vector3Dd surfaceNormal = localShading.normal;
    double outR = localShading.color.r(), outG = localShading.color.g(), outB = localShading.color.b();
    double nx = surfaceNormal.x(), ny = surfaceNormal.y(), nz = surfaceNormal.z();

    double kr = material->getReflectionCoefficient();
    if ( kr > 0 && recursions > 0 ) {
        double t = viewX*nx + viewY*ny + viewZ*nz;
        if ( t > 0 ) {
            double twoT = 2*t;
            double rx = twoT*nx - viewX, ry = twoT*ny - viewY, rz = twoT*nz - viewZ;
            Vector3Dd reflect(rx, ry, rz);
            Vector3Dd poffset(info->point.x() + VSDK::EPSILON*rx, info->point.y() + VSDK::EPSILON*ry, info->point.z() + VSDK::EPSILON*rz);
            Ray reflectedRay(poffset, reflect);

            RayHit* reflectedHit = &workspace.reflectionHits[recursionLevel];
            int nearestObjectIndex = selectNearestThingInRayDirection(reflectedRay, objects, reflectedHit, &workspace.traversalCandidateHit);
            if ( nearestObjectIndex >= 0 ) {
                SimpleBody* nearestObject = objects.get(nearestObjectIndex);
                const SceneObjectRenderData& objectData = cache.objects.get(nearestObjectIndex);
                RayHit* subInfo = &workspace.shadingHits[recursionLevel + 1];
                Ray reflectedHitRay = reflectedRay.withT(reflectedHit->getHitDistance());
                prepareSurfaceHit(nearestObject, objectData, reflectedHitRay, subInfo);
                ColorRgb rcolor = evaluateIlluminationModel(subInfo, -reflectedRay.getDirection().x(), -reflectedRay.getDirection().y(), -reflectedRay.getDirection().z(), lights, objects, cache, background, resolveMaterial(subInfo, objectData), renderContext, recursions - 1, recursionLevel + 1);
                outR += rcolor.r() * kr; outG += rcolor.g() * kr; outB += rcolor.b() * kr;
            }
            else {
                ColorRgb reflectedBackground = background->colorInDireccion(reflect);
                outR += reflectedBackground.r() * kr; outG += reflectedBackground.g() * kr; outB += reflectedBackground.b() * kr;
            }
        }
    }

    return ColorRgb(outR > 1 ? 1 : outR, outG > 1 ? 1 : outG, outB > 1 ? 1 : outB);
}

int SimpleRaytracer::selectNearestThingInRayDirection(const Ray& inRay,java::ArrayList<SimpleBody*>& bodies,RayHit* outHit,RayHit* candidateHit)
{
    double nearestDistance = 1e308; int nearestIndex = -1;
    candidateHit->setStoreRay(false);
    RaytraceStatistics::recordSceneTraversal();
    for ( long int i=0; i<bodies.size(); i++ ) {
        candidateHit->resetForDistanceOnly();
        RaytraceStatistics::recordObjectIntersectionTest();
        if ( bodies.get(i)->doIntersectionFirstHit(inRay, candidateHit) ) {
            double d = candidateHit->getHitDistance();
            if ( d < nearestDistance && d > VSDK::EPSILON ) { nearestDistance = d; nearestIndex = (int)i; }
        }
    }
    if ( nearestIndex >= 0 && outHit != 0 ) { outHit->resetForDistanceOnly(); outHit->setHitDistance(nearestDistance); }
    return nearestIndex;
}

ColorRgb SimpleRaytracer::followRayPath(const Ray& inRay,java::ArrayList<SimpleBody*>& bodies,java::ArrayList<Light*>& lights,Background* background,RenderContext& renderContext,const SceneRenderCache& cache)
{
    RayHit* hitInfo = &workspace.nearestHit;
    int nearestIndex = selectNearestThingInRayDirection(inRay, bodies, hitInfo, &workspace.traversalCandidateHit);
    if ( nearestIndex >= 0 ) {
        SimpleBody* nearestObject = bodies.get(nearestIndex);
        const SceneObjectRenderData& objectData = cache.objects.get(nearestIndex);
        Ray primaryHitRay = inRay.withT(hitInfo->getHitDistance());
        RayHit* shadingInfo = &workspace.shadingHits[0];
        prepareSurfaceHit(nearestObject, objectData, primaryHitRay, shadingInfo);
        return evaluateIlluminationModel(shadingInfo, -inRay.getDirection().x(), -inRay.getDirection().y(), -inRay.getDirection().z(), lights, bodies, cache, background, resolveMaterial(shadingInfo, objectData), renderContext, MAX_RECURSION_LEVEL, 0);
    }
    // No hit: the primary distance is read to export depth buffers
    hitInfo->setHitDistance(java::Double::POSITIVE_INFINITY);
    return background->colorInDireccion(inRay.getDirection());
}

void SimpleRaytracer::execute(RGBImageUncompressed* inoutViewport,const RendererConfiguration* q,SimpleSceneSnapshot* sceneSnapshot,ProgressMonitor* report)
{ execute(inoutViewport, q, sceneSnapshot, report, 0, 0, 0, inoutViewport->getXSize(), inoutViewport->getYSize()); }

void SimpleRaytracer::execute(RGBImageUncompressed* inoutViewport,const RendererConfiguration* q,SimpleSceneSnapshot* sceneSnapshot,ProgressMonitor* report,ZBuffer* depthmap)
{ execute(inoutViewport, q, sceneSnapshot, report, depthmap, 0, 0, inoutViewport->getXSize(), inoutViewport->getYSize()); }

void SimpleRaytracer::execute(RGBImageUncompressed* inoutViewport,const RendererConfiguration* q,SimpleSceneSnapshot* sceneSnapshot,ProgressMonitor* liveReport,ZBuffer* outDepthmap,int limx1,int limy1,int limx2,int limy2)
{ execute(inoutViewport, q, sceneSnapshot, liveReport, outDepthmap, DepthBufferMode::RAY_DISTANCE, limx1, limy1, limx2, limy2); }

void SimpleRaytracer::execute(RGBImageUncompressed* inoutViewport,const RendererConfiguration* q,SimpleSceneSnapshot* sceneSnapshot,ProgressMonitor* liveReport,ZBuffer* outDepthmap,DepthBufferMode depthMode,int limx1,int limy1,int limx2,int limy2)
{
    if ( outDepthmap != 0 && depthMode != DepthBufferMode::NONE ) {
        DepthBufferEncoder depthEncoder(depthMode, sceneSnapshot->getCameraSnapshot());
        execute(inoutViewport, q, sceneSnapshot, liveReport, outDepthmap, &depthEncoder, limx1, limy1, limx2, limy2);
        return;
    }
    execute(inoutViewport, q, sceneSnapshot, liveReport, outDepthmap, (const DepthBufferEncoder*)0, limx1, limy1, limx2, limy2);
}

void SimpleRaytracer::execute(RGBImageUncompressed* inoutViewport,const RendererConfiguration* q,SimpleSceneSnapshot* sceneSnapshot,ProgressMonitor* liveReport,ZBuffer* outDepthmap,const DepthBufferEncoder* depthEncoder,int limx1,int limy1,int limx2,int limy2)
{ execute(inoutViewport, q, sceneSnapshot->getSimpleBodies(), sceneSnapshot->getLights(), sceneSnapshot->getBackground(), sceneSnapshot->getCameraSnapshot(), liveReport, depthEncoder != 0 ? outDepthmap : 0, depthEncoder, limx1, limy1, limx2, limy2); }

void SimpleRaytracer::execute(RGBImageUncompressed* inoutViewport,const RendererConfiguration* q,java::ArrayList<SimpleBody*>& bodies,java::ArrayList<Light*>& lights,Background* bg,const CameraSnapshot* cameraSnapshot,ProgressMonitor* liveReport,ZBuffer* outDepthmap,const DepthBufferEncoder* depthEncoder,int limx1,int limy1,int limx2,int limy2)
{
    RenderContext renderContext = buildRenderContext(q, lights);
    SceneRenderCache cache;
    for ( long int i=0; i<bodies.size(); i++ ) {
        SceneObjectRenderData d;
        d.material = bodies.get(i)->getMaterial();
        d.texture = renderContext.textureEnabled ? bodies.get(i)->getTexture() : 0;
        d.normalMap = renderContext.bumpMappingEnabled ? bodies.get(i)->getNormalMap() : 0;
        d.detailMask = buildSurfaceDetailMask(d.material, d.texture, d.normalMap, renderContext);
        cache.objects.add(d);
    }

    java::ArrayList<long long> versions;
    captureBodyVersions(bodies, versions);
    RasterTileGenerator tileGenerator(TILE_STRATEGY, inoutViewport, limx1, limy1, limx2 - limx1, limy2 - limy1, TILE_WORKERS_HINT);
    const java::ArrayList<RasterTileArea>& tiles = tileGenerator.getTiles();
    RGBPixel outputPixel;

    if ( liveReport ) liveReport->begin();
    for ( long int ti=0; ti<tiles.size(); ti++ ) {
        const RasterTileArea& tile = tiles.get(ti);
        Image* tileImage = tile.getImage();
        for ( int y = tile.getStartY(); y < tile.getEndY(); y++ ) {
            assertSceneUnmodifiedDuringRender(versions, bodies);
            if ( liveReport ) liveReport->update(0, inoutViewport->getYSize(), y);
            for ( int x = tile.getStartX(); x < tile.getEndX(); x++ ) {
                RaytraceStatistics::recordPrimaryRay();
                Ray ray = generateRay(cameraSnapshot, x, y);
                ColorRgb color = followRayPath(ray, bodies, lights, bg, renderContext, cache);
                if ( outDepthmap ) outDepthmap->setDepth(x, y, depthEncoder->encode(ray.getOrigin(), ray.getDirection(), workspace.nearestHit.getHitDistance()));
                outputPixel.r = (char)(255 * color.r()); outputPixel.g = (char)(255 * color.g()); outputPixel.b = (char)(255 * color.b());
                tileImage->putPixelRgb(x, y, &outputPixel);
            }
        }
    }
    if ( liveReport ) liveReport->end();
    delete renderContext.localShader;
}

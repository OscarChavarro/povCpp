#ifndef __SIMPLE_RAYTRACER__
#define __SIMPLE_RAYTRACER__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/render/RenderContext.h"
#include "vsdk/toolkit/render/RenderingElement.h"
#include "vsdk/toolkit/render/TraceWorkspace.h"
#include "vsdk/toolkit/render/raytracing/RasterTileGenerationStrategy.h"
#include "vsdk/toolkit/render/raytracing/DepthBufferMode.h"
class Ray;
class RayHit;
class ColorRgb;
class RGBImageUncompressed;
class RGBPixel;
class ZBuffer;
class RendererConfiguration;
class ProgressMonitor;
class SimpleSceneSnapshot;
class SimpleBody;
class Light;
class Background;
class SimpleMaterial;
class Shader;
class Image;
class NormalMap;
class CameraSnapshot;
class DepthBufferEncoder;

class SimpleRaytracer : public RenderingElement {
private:
    static const int MAX_RECURSION_LEVEL = TraceWorkspace::DEFAULT_MAX_RECURSION_LEVEL;
    static const RasterTileGenerationStrategy TILE_STRATEGY = RasterTileGenerationStrategy::SERIAL;
    static const int TILE_WORKERS_HINT = 1;

    struct SceneObjectRenderData {
        SimpleMaterial* material;
        Image* texture;
        NormalMap* normalMap;
        int detailMask;
        SceneObjectRenderData() : material(0), texture(0), normalMap(0), detailMask(0) {}
    };

    struct SceneRenderCache {
        java::ArrayList<SceneObjectRenderData> objects;
    };

    TraceWorkspace workspace;

    static bool hasNonAmbientLights(java::ArrayList<Light*>& lights);
    static bool isReflective(SimpleMaterial* material);
    static RenderContext buildRenderContext(const RendererConfiguration* qualitySelection,java::ArrayList<Light*>& lights);
    static int buildSurfaceDetailMask(SimpleMaterial* material, Image* texture, NormalMap* normalMap, const RenderContext& renderContext);
    static void captureBodyVersions(java::ArrayList<SimpleBody*>& bodies, java::ArrayList<long long>& out);
    static void assertSceneUnmodifiedDuringRender(java::ArrayList<long long>& expectedBodyVersions,java::ArrayList<SimpleBody*>& bodies);
    static Ray generateRay(const CameraSnapshot* cameraSnapshot, int x, int y);

    void prepareSurfaceHit(SimpleBody* nearestObject, const SceneObjectRenderData& objectData, const Ray& hitRay, RayHit* outHit);
    static SimpleMaterial* resolveMaterial(RayHit* hit, const SceneObjectRenderData& objectData);
    ColorRgb evaluateIlluminationModel(RayHit* info,double viewX,double viewY,double viewZ,java::ArrayList<Light*>& lights,java::ArrayList<SimpleBody*>& objects,const SceneRenderCache& sceneRenderCache,Background* background,SimpleMaterial* material,RenderContext& renderContext,int recursions,int recursionLevel);
    int selectNearestThingInRayDirection(const Ray& inRay,java::ArrayList<SimpleBody*>& inSimpleBodiesArray,RayHit* outHit,RayHit* candidateHit);
    ColorRgb followRayPath(const Ray& inRay,java::ArrayList<SimpleBody*>& inSimpleBodiesArray,java::ArrayList<Light*>& inLightsArray,Background* in_background,RenderContext& renderContext,const SceneRenderCache& sceneRenderCache);
    void execute(RGBImageUncompressed* inoutViewport,const RendererConfiguration* inQualitySelection,java::ArrayList<SimpleBody*>& inSimpleBodiesArray,java::ArrayList<Light*>& inLightsArray,Background* inBackground,const CameraSnapshot* cameraSnapshot,ProgressMonitor* liveReport,ZBuffer* outDepthmap,const DepthBufferEncoder* depthEncoder,int limx1,int limy1,int limx2,int limy2);

public:
    SimpleRaytracer();
    virtual ~SimpleRaytracer() {}

    void execute(RGBImageUncompressed* inoutViewport,const RendererConfiguration* inQualitySelection,SimpleSceneSnapshot* sceneSnapshot,ProgressMonitor* report);
    void execute(RGBImageUncompressed* inoutViewport,const RendererConfiguration* inQualitySelection,SimpleSceneSnapshot* sceneSnapshot,ProgressMonitor* report,ZBuffer* depthmap);
    void execute(RGBImageUncompressed* inoutViewport,const RendererConfiguration* inQualitySelection,SimpleSceneSnapshot* sceneSnapshot,ProgressMonitor* liveReport,ZBuffer* outDepthmap,int limx1,int limy1,int limx2,int limy2);

    /**
    Raytraces a rectangular area of the image, optionally exporting a depth
    buffer of the primary rays.
    @param inoutViewport image to fill; its size gives the resolution
    @param inQualitySelection quality settings
    @param sceneSnapshot scene to render, seen from its camera snapshot
    @param liveReport progress monitor, or null
    @param outDepthmap depth buffer of the same size as the image, or null
    @param depthMode kind of values written in `outDepthmap` (see
    `DepthBufferMode`); `NONE` leaves it untouched
    @param limx1 first column of the area
    @param limy1 first row of the area
    @param limx2 column after the last one of the area
    @param limy2 row after the last one of the area
    */
    void execute(RGBImageUncompressed* inoutViewport,const RendererConfiguration* inQualitySelection,SimpleSceneSnapshot* sceneSnapshot,ProgressMonitor* liveReport,ZBuffer* outDepthmap,DepthBufferMode depthMode,int limx1,int limy1,int limx2,int limy2);

    /**
    Raytraces a rectangular area of the image, exporting the depth buffer of
    the primary rays with the given encoder (with its own depth range).
    @param outDepthmap depth buffer of the same size as the image, or null
    @param depthEncoder converts the primary ray distances in depth values,
    or null to not export depth
    (the other parameters as in the overload with `DepthBufferMode`)
    */
    void execute(RGBImageUncompressed* inoutViewport,const RendererConfiguration* inQualitySelection,SimpleSceneSnapshot* sceneSnapshot,ProgressMonitor* liveReport,ZBuffer* outDepthmap,const DepthBufferEncoder* depthEncoder,int limx1,int limy1,int limx2,int limy2);
};

#endif

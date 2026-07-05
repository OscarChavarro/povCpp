#ifndef __RENDER_CONTEXT__
#define __RENDER_CONTEXT__

#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"
#include "render/PovRayRendererConfiguration.h"
#include "render/shaders/PovRayRenderStatistics.h"
#include "environment/scene/Scene.h"
#include "render/bakedScene/BakedScene.h"

class RenderContext {
  private:
    const PovRayRendererConfiguration &config;
    PovRayRenderStatistics &statistics;
    const Scene &scene;
    TextureUtils &textureUtils;
    const BakedScene *bakedScene = nullptr;

  public:
    RenderContext(
        const PovRayRendererConfiguration &cfg,
        PovRayRenderStatistics &stats,
        const Scene &scn,
        TextureUtils &tex)
        : config(cfg), statistics(stats), scene(scn), textureUtils(tex) {}

    const PovRayRendererConfiguration &getConfig() const { return config; }
    PovRayRenderStatistics &getStatistics() { return statistics; }
    const PovRayRenderStatistics &getStatistics() const { return statistics; }
    const Scene &getScene() const { return scene; }
    TextureUtils &getTextureUtils() { return textureUtils; }
    void setBakedScene(const BakedScene &b) { bakedScene = &b; }
    const BakedScene &getBakedScene() const { return *bakedScene; }
};

#endif

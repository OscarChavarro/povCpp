#ifndef __RENDER_CONTEXT__
#define __RENDER_CONTEXT__

#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"
#include "environment/scene/Scene.h"
#include "render/RenderContext.h"
#include "render/bakedScene/BakedScene.h"

class RenderContext {
  private:
    const PovRayRendererConfiguration &config;
    Statistics &statistics;
    const Scene &scene;
    TextureUtils &textureUtils;
    const BakedScene *bakedScene = nullptr;

  public:
    RenderContext(
        const PovRayRendererConfiguration &cfg,
        Statistics &stats,
        const Scene &scn,
        TextureUtils &tex)
        : config(cfg), statistics(stats), scene(scn), textureUtils(tex) {}

    const PovRayRendererConfiguration &getConfig() const { return config; }
    Statistics &getStatistics() { return statistics; }
    const Statistics &getStatistics() const { return statistics; }
    const Scene &getScene() const { return scene; }
    TextureUtils &getTextureUtils() { return textureUtils; }
    void setBakedScene(const BakedScene &b) { bakedScene = &b; }
    const BakedScene &getBakedScene() const { return *bakedScene; }
};

#endif

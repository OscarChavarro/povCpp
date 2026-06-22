#ifndef __RENDER_CONTEXT__
#define __RENDER_CONTEXT__

#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"
#include "environment/scene/Scene.h"
#include "render/RenderContext.h"

class RenderContext {
  private:
    const RenderingConfiguration &config;
    Statistics &statistics;
    const Scene &scene;
    TextureUtils &textureUtils;

  public:
    RenderContext(
        const RenderingConfiguration &cfg,
        Statistics &stats,
        const Scene &scn,
        TextureUtils &tex)
        : config(cfg), statistics(stats), scene(scn), textureUtils(tex) {}

    const RenderingConfiguration &getConfig() const { return config; }
    Statistics &getStatistics() { return statistics; }
    const Statistics &getStatistics() const { return statistics; }
    const Scene &getScene() const { return scene; }
    TextureUtils &getTextureUtils() { return textureUtils; }
};

#endif

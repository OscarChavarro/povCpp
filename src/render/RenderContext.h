#ifndef __RENDER_CONTEXT__
#define __RENDER_CONTEXT__

#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"
#include "common/RenderRuntimeState.h"
#include "environment/scene/Scene.h"
#include "render/RenderContext.h"

class RenderContext {
  private:
    const RenderingConfiguration &config;
    Statistics &statistics;
    const Scene &scene;
    RenderRuntimeState &runtime;
    TextureUtils &textureUtils;

  public:
    RenderContext(
        const RenderingConfiguration &cfg,
        Statistics &stats,
        const Scene &scn,
        RenderRuntimeState &rt,
        TextureUtils &tex)
        : config(cfg), statistics(stats), scene(scn), runtime(rt), textureUtils(tex) {}

    const RenderingConfiguration &getConfig() const { return config; }
    Statistics &getStatistics() { return statistics; }
    const Statistics &getStatistics() const { return statistics; }
    const Scene &getScene() const { return scene; }
    RenderRuntimeState &getRuntime() { return runtime; }
    TextureUtils &getTextureUtils() { return textureUtils; }
};

#endif

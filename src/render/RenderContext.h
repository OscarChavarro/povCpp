#ifndef __RENDER_CONTEXT__
#define __RENDER_CONTEXT__

class RenderingConfiguration;
class Statistics;
class Scene;
class RenderRuntimeState;
class TextureUtils;

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

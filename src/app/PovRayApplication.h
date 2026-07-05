#ifndef __POV_RAY_APPLICATION__
#define __POV_RAY_APPLICATION__

#include "io/context/RenderRuntimeState.h"
#include "io/image/ImageOutput.h"
#include "render/RenderEngine.h"

class PovRayApplication {
  private:
    PovRayRendererConfiguration configuration;
    FileLocator fileLocator;
    PovRayRenderStatistics statistics;
    RenderRuntimeState runtimeState;
    TextureUtils textureUtils;
    Scene scene;
    RenderEngine engine;
    RenderContext context;
    ImageOutput *selectedImageOutput;

    void printProgress(const char *message);
    void printStatistics(
        const PovRayRenderStatistics &stats,
        const Scene &frame,
        const PovRayRendererConfiguration &inputConfiguration);

    void initializeFromCommandLine(int argc, char *argv[]);
    void configureOutputTarget();
    void parseSceneDescription();
    void prepareRendering();
    void runRenderLoop();
    void finalizeRun();
    void initVars();
    void closeAll();

  public:
    PovRayApplication()
        : context(configuration, statistics, scene, textureUtils) {
        engine.setScene(&scene);
        engine.setContext(&context);
        engine.setMaxTraceLevel(runtimeState.getMaxTraceLevel());
        engine.setStopFlag(runtimeState.getStopFlag());
    }

    void run(int argc, char *argv[]);
};

#endif

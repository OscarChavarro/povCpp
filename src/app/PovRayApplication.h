#ifndef __POV_RAY_APPLICATION__
#define __POV_RAY_APPLICATION__

#include "java/util/ArrayList.txx"
#include "common/statistics/Statistics.h"
#include "common/RenderRuntimeState.h"
#include "environment/material/RendererConfiguration.h"
#include "environment/scene/Scene.h"
#include "io/binaryIo/FileLocator.h"
#include "io/image/ImageOutput.h"
#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"
#include "render/RenderEngine.h"
#include "render/RenderContext.h"

class PovRayApplication {
  private:
    RenderingConfiguration configuration;
    FileLocator fileLocator;
    Statistics statistics;
    RenderRuntimeState runtimeState;
    TextureUtils textureUtils;
    Scene scene;
    RenderEngine engine;
    RenderContext context;
    ImageOutput *selectedImageOutput;

    void printProgress(const char *message);
    void printStatistics(
        const Statistics &stats,
        const Scene &frame,
        const RenderingConfiguration &inputConfiguration);

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
        : context(configuration, statistics, scene, runtimeState, textureUtils) {
        engine.setScene(&scene);
        engine.setContext(&context);
    }

    void run(int argc, char *argv[]);
};

#endif

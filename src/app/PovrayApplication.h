#ifndef __POVRAY_APPLICATION__
#define __POVRAY_APPLICATION__

#include "common/statistics/Statistics.h"
#include "common/RenderRuntimeState.h"
#include "environment/material/RendererConfiguration.h"
#include "environment/scene/Scene.h"
#include "io/image/ImageOutput.h"
#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"
#include "render/RenderEngine.h"

class PovrayApplication {
  private:
    RenderingConfiguration configuration;
    Statistics statistics;
    RenderRuntimeState runtimeState;
    TextureUtils textureUtils;
    Scene scene;
    ImageOutput *selectedImageOutput;

    void printStatistics(
        const Statistics &stats,
        const Scene &frame,
        const RenderingConfiguration &configuration);

    void initializeFromCommandLine(int argc, char *argv[]);
    void configureOutputTarget();
    void parseSceneDescription();
    void prepareRendering();
    void runRenderLoop();
    void finalizeRun();
    void initVars();
    void closeAll();
    void printCredits();

  public:
    PovrayApplication() {
        RenderingConfiguration::installActive(&configuration);
        Statistics::installActive(&statistics);
        RenderRuntimeState::installActive(&runtimeState);
        TextureUtils::installActive(&textureUtils);
        RenderEngine::setScene(&scene);
    }

    void run(int argc, char *argv[]);
};

#endif

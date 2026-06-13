#ifndef __POVRAY_APPLICATION_H__
#define __POVRAY_APPLICATION_H__

#include "common/statistics/Statistics.h"
#include "environment/material/RendererConfiguration.h"
#include "environment/scene/SceneFrame.h"
#include "io/image/ImageOutput.h"

class PovrayApplication {
  private:
    static ImageOutput *selectedImageOutput;

    static void printStatistics(
        const Statistics &stats,
        const RenderFrame &frame,
        const RenderingConfiguration &configuration);

    static void initializeFromCommandLine(int argc, char *argv[]);
    static void configureOutputTarget();
    static void parseSceneDescription();
    static void prepareRendering();
    static void runRenderLoop();
    static void finalizeRun();
    static void initVars();
    static void closeAll();
    static void printCredits();

  public:
    static void run(int argc, char *argv[]);
};

#endif

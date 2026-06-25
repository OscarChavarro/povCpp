#ifndef __COMMAND_LINE_OPTIONS__
#define __COMMAND_LINE_OPTIONS__

#include "environment/material/PovRayRendererConfiguration.h"
#include "environment/scene/Scene.h"
#include "io/binaryIo/FileLocator.h"

class CommandLineOptions {
  private:
    static void readOptions(const char *optionLine, PovRayRendererConfiguration &config,
        FileLocator &fileLocator, Scene &scene, bool &inFlag, bool &outFlag);
    static void parseOption(const char *optionString, PovRayRendererConfiguration &config,
        FileLocator &fileLocator, Scene &scene, bool &inFlag, bool &outFlag);
    static void parseFileName(const char *fileName, PovRayRendererConfiguration &config,
        FileLocator &fileLocator, Scene &scene, int &numberOfFiles, bool &inFlag, bool &outFlag);

  public:
    static void usage();
    static void loadDefaults(PovRayRendererConfiguration &config, FileLocator &fileLocator,
        Scene &scene);
    static void parseArguments(int argc, char *argv[], PovRayRendererConfiguration &config,
        FileLocator &fileLocator, Scene &scene);
};

#endif

#ifndef __POV_APP_H__
#define __POV_APP_H__

#include "common/FrameConfig.h"

class PovApp {
  public:
    static void initializeFromCommandLine(int argc, char *argv[]);
    static void configureOutputTarget();
    static void parseSceneDescription();
    static void prepareRendering();
    static void runRenderLoop();
    static void finalizeRun();

    static void usage();
    static void initVars();
    static void closeAll();
    static void getDefaults();
    static void readOptions(char *optionLine);
    static void parseOption(char *optionString);
    static void printOptions();
    static void parseFileName(char *fileName);
    static void printStats();
    static FILE *locateFile(const char *filename, const char *mode);
    static void printCredits();
};

#endif

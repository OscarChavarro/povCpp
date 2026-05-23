#ifndef __POV_APP_H__
#define __POV_APP_H__

#include "common/FrameConfig.h"

class FileHandle;
class RGBAColor;

class PovApp {
  public:
    static constexpr int OUTPUT_READ_MODE = 0;
    static constexpr int OUTPUT_WRITE_MODE = 1;
    static constexpr int OUTPUT_APPEND_MODE = 2;

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
    static char *defaultOutputFileName(FileHandle *handle);
    static int openOutputFile(FileHandle *handle, char *name, int *width,
        int *height, int bufferSize, int mode);
    static void writeOutputLine(
        FileHandle *handle, RGBAColor *lineData, int lineNumber);
    static int readOutputLine(
        FileHandle *handle, RGBAColor *lineData, int *lineNumber);
    static void closeOutputFile(FileHandle *handle);
};

#endif

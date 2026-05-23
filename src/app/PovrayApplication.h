#ifndef __POVRAY_APPLICATION_H__
#define __POVRAY_APPLICATION_H__

class PovrayApplication {
  public:
    static void run(int argc, char *argv[]);

  private:
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
    static void printCredits();
};

#endif

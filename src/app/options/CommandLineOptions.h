#ifndef __COMMAND_LINE_OPTIONS__
#define __COMMAND_LINE_OPTIONS__

class RenderingConfiguration;
class Scene;

class CommandLineOptions {
  private:
    static void readOptions(const char *optionLine, RenderingConfiguration &config,
        Scene &scene, bool &inFlag, bool &outFlag);
    static void parseOption(const char *optionString, RenderingConfiguration &config,
        Scene &scene, bool &inFlag, bool &outFlag);
    static void parseFileName(const char *fileName, RenderingConfiguration &config,
        Scene &scene, int &numberOfFiles, bool &inFlag, bool &outFlag);

  public:
    static void usage();
    static void loadDefaults(RenderingConfiguration &config, Scene &scene);
    static void parseArguments(int argc, char *argv[], RenderingConfiguration &config,
        Scene &scene);
};

#endif

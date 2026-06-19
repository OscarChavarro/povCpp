#ifndef __COMMAND_LINE_OPTIONS__
#define __COMMAND_LINE_OPTIONS__

class RenderingConfiguration;
class FileLocator;
class Scene;

class CommandLineOptions {
  private:
    static void readOptions(const char *optionLine, RenderingConfiguration &config,
        FileLocator &fileLocator, Scene &scene, bool &inFlag, bool &outFlag);
    static void parseOption(const char *optionString, RenderingConfiguration &config,
        FileLocator &fileLocator, Scene &scene, bool &inFlag, bool &outFlag);
    static void parseFileName(const char *fileName, RenderingConfiguration &config,
        FileLocator &fileLocator, Scene &scene, int &numberOfFiles, bool &inFlag, bool &outFlag);

  public:
    static void usage();
    static void loadDefaults(RenderingConfiguration &config, FileLocator &fileLocator,
        Scene &scene);
    static void parseArguments(int argc, char *argv[], RenderingConfiguration &config,
        FileLocator &fileLocator, Scene &scene);
};

#endif

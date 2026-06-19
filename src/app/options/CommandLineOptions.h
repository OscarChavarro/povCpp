#ifndef __COMMAND_LINE_OPTIONS__
#define __COMMAND_LINE_OPTIONS__

class RenderingConfiguration;

class CommandLineOptions {
  private:
    static int numberOfFiles;
    static bool inFlag;
    static bool outFlag;

    static void readOptions(const char *optionLine, RenderingConfiguration &config);
    static void parseOption(const char *optionString, RenderingConfiguration &config);
    static void parseFileName(const char *fileName, RenderingConfiguration &config);

  public:
    static void reset();
    static void usage();
    static void loadDefaults(RenderingConfiguration &config);
    static void parseArguments(int argc, char *argv[], RenderingConfiguration &config);
};

#endif

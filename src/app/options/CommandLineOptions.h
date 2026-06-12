#ifndef __COMMAND_LINE_OPTIONS_H__
#define __COMMAND_LINE_OPTIONS_H__

class CommandLineOptions {
  private:
    static int numberOfFiles;
    static bool inFlag;
    static bool outFlag;

    static void readOptions(const char *optionLine);
    static void parseOption(const char *optionString);
    static void parseFileName(const char *fileName);

  public:
    static void reset();
    static void usage();
    static void loadDefaults();
    static void parseArguments(int argc, char *argv[]);
};

#endif

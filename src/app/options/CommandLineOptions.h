#ifndef __COMMAND_LINE_OPTIONS_H__
#define __COMMAND_LINE_OPTIONS_H__

class CommandLineOptions {
  private:
    static int numberOfFiles;
    static bool inFlag;
    static bool outFlag;

    static void readOptions(char *optionLine);
    static void parseOption(char *optionString);
    static void parseFileName(char *fileName);

  public:
    static void reset();
    static void usage();
    static void loadDefaults();
    static void parseArguments(int argc, char *argv[]);
    static void printOptions();
};

#endif

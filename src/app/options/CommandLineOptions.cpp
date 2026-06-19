#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "vsdk/toolkit/common/logging/Logger.h"
#include "environment/material/RendererConfiguration.h"
#include "environment/scene/Scene.h"
#include "io/binaryIo/FileLocator.h"
#include "io/pov/lexer/Tokenizer.h"
#include "app/options/CommandLineOptions.h"

static constexpr int MAX_FILE_NAMES = 1;

// Print out usage error message
void
CommandLineOptions::usage()
{
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\nUsage:");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n    povray  [+/-] Option1 [+/-] Option2 ...");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n Options: ");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n     dxy = display in format x, using palette option y");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n     vx  = verbose in format x");
    // Logger::info("\n     @filename  = verbose to file name -- see docs");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n     p  = pause before exit");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n     x  = enable early exit by key hit");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n     fx = write output file in format x");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n            ft - Uncompressed Targa-24  fd - DKB/QRT "                     "Dump  fr - 3 Raw Files");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n     a  = perform antialiasing");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n     c  = continue aborted trace");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n     qx = image quality 0=rough, 9=full");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n     l<pathname> = library path prefix");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n     wxxx = width of the screen");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n     hxxx = height of the screen");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n     sxxx = start at line number xxx");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n     exxx = end at line number xxx");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n     bxxx = Use xxx kilobytes for output file buffer space");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n     i<filename> = input file name");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n     o<filename> = output file name");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n  Ex: +l\\povray\\include +iscene.pov +oscene.tga +w320 "                     "+h200 +d -v +x");
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n  Ex: +iscene.pov +oscene.tga +w160 +h200 +v -d +x");
    Logger::reportMessage("CommandLineOptions", Logger::FATAL_ERROR, "", "\n");
}

// Read the default parameters from povray.def
void
CommandLineOptions::loadDefaults(RenderingConfiguration &config, Scene &scene)
{
    FILE *defaultsFile;
    char optionString[256];
    char *Option_String_Ptr;
    /**
    READ_ENV_VAR_? should be defined in config.h
    Only one READ_ENV_VAR_? should ever be defined.
    This allows some machines to read environment variable before
    reading povray.def and others to do it after depending on the
    operating system. IBM-PC is before. Default is after if not
    defined in config.h. CDW 2/92
    */
    // Set Diskwrite as default
    config.setOptionFlags(RenderingConfiguration::DISKWRITE);
    config.setOutputFormat(RenderingConfiguration::DEFAULT_OUTPUT_FORMAT);

    if ((Option_String_Ptr = getenv("POVRAYOPT")) != nullptr) {
        bool inFlag = false;
        bool outFlag = false;
        readOptions(Option_String_Ptr, config, scene, inFlag, outFlag);
    }
    if ((defaultsFile = config.getFileLocator().locate("povray.def", "r")) != nullptr) {
        bool inFlag = false;
        bool outFlag = false;
        while (fgets(optionString, 256, defaultsFile) != nullptr) {
            readOptions(optionString, config, scene, inFlag, outFlag);
        }
        fclose(defaultsFile);
    }
}

void
CommandLineOptions::parseArguments(int argc, char *argv[], RenderingConfiguration &config,
    Scene &scene)
{
    int numberOfFiles = 0;
    bool inFlag = false;
    bool outFlag = false;
    for (int i = 1; i < argc; i++) {
        if ((*argv[i] == '+') || (*argv[i] == '-')) {
            parseOption(argv[i], config, scene, inFlag, outFlag);
        } else {
            parseFileName(argv[i], config, scene, numberOfFiles, inFlag, outFlag);
        }
    }
}

void
CommandLineOptions::readOptions(const char *optionLine, RenderingConfiguration &config,
    Scene &scene, bool &inFlag, bool &outFlag)
{
    int c;
    int stringIndex;
    bool optionStarted;
    short optionLineIndex = 0;
    char optionString[80];

    stringIndex = 0;
    optionStarted = false;
    while ((c = optionLine[optionLineIndex++]) != '\0') {
        if (optionStarted) {
            if (isspace(c)) {
                optionString[stringIndex] = '\0';
                parseOption(optionString, config, scene, inFlag, outFlag);
                optionStarted = false;
                stringIndex = 0;
            } else {
                optionString[stringIndex++] = (char)c;
            }

        } else // Option_Started
            if ((c == (int)'-') || (c == (int)'+')) {
                stringIndex = 0;
                optionString[stringIndex++] = (char)c;
                optionStarted = true;
            } else if (!isspace(c)) {
                {
                    char _logMsg[1024];
                    snprintf(_logMsg, sizeof(_logMsg), "\nBad default file format.  Offending char: (%c), val: "                     "%d.\n", (char)c, c);
                    Logger::reportMessage("CommandLineOptions", Logger::FATAL_ERROR, "", _logMsg);
                }
            }
    }

    if (optionStarted) {
        optionString[stringIndex] = '\0';
        parseOption(optionString, config, scene, inFlag, outFlag);
    }
}

// Parse the command line parameters
void
CommandLineOptions::parseOption(const char *optionString, RenderingConfiguration &config,
    Scene &scene, bool &inFlag, bool &outFlag)
{
    bool addOption;
    unsigned int optionNumber = 0;
    double threshold;

    inFlag = outFlag = false; // If these flags aren't immediately used, reset
                              // them on next -/+ option!
    if (*(optionString++) == '-') {
        addOption = false;
    } else {
        addOption = true;
    }

    switch (*optionString) {
    case 'B':
    case 'b':
        {
            int fileBufferSize;
            sscanf(&optionString[1], "%d", &fileBufferSize);
            fileBufferSize *= 1024;
            if (fileBufferSize < BUFSIZ) {
                fileBufferSize = BUFSIZ;
            }
            config.setFileBufferSize(fileBufferSize);
        }
        optionNumber = 0;
        break;

    case 'C':
    case 'c':
        optionNumber = RenderingConfiguration::CONTINUE_TRACE;
        break;

    case 'D':
    case 'd':
        optionNumber = RenderingConfiguration::DISPLAY;
        config.setDisplayFormat('0');
        config.setPaletteOption('3');
        if (optionString[1] != '\0') {
            config.setDisplayFormat((char)toupper(optionString[1]));
        }

        if (optionString[1] != '\0' && optionString[2] != '\0') {
            config.setPaletteOption((char)toupper(optionString[2]));
        }
        break;

    case '@':
        optionNumber = RenderingConfiguration::VERBOSE_FILE;
        if (optionString[1] == '\0') {
            config.setStatFileName("POVSTAT.OUT");
        } else {
            config.setStatFileName(&optionString[1]);
        }
        break;
    case 'V':
    case 'v':
        optionNumber = RenderingConfiguration::VERBOSE;
        config.setVerboseFormat((char)toupper(optionString[1]));
        if (config.getVerboseFormat() == '\0') {
            config.setVerboseFormat('1');
        }
        break;

    case 'W':
    case 'w':
        sscanf(&optionString[1], "%d", &scene.getScreenWidth());
        optionNumber = 0;
        break;

    case 'H':
    case 'h':
        sscanf(&optionString[1], "%d", &scene.getScreenHeight());
        optionNumber = 0;
        break;

    case 'F':
    case 'f':
        optionNumber = RenderingConfiguration::DISKWRITE;
        if (isupper(optionString[1])) {
            config.setOutputFormat((char)tolower(optionString[1]));
        } else {
            config.setOutputFormat(optionString[1]);
        }

        // Default the output format to the default in the config file
        if (config.getOutputFormat() == '\0') {
            config.setOutputFormat(RenderingConfiguration::DEFAULT_OUTPUT_FORMAT);
        }
        break;

    case 'P':
    case 'p':
        optionNumber = RenderingConfiguration::PROMPTEXIT;
        break;

    case 'I':
    case 'i':
        if (optionString[1] == '\0') {
            inFlag = true;
        } else {
            config.setInputFileName(&optionString[1]);
        }
        optionNumber = 0;
        break;

    case 'O':
    case 'o':
        if (optionString[1] == '\0') {
            outFlag = true;
        } else {
            config.setOutputFileName(&optionString[1]);
        }
        optionNumber = 0;
        break;

    case 'A':
    case 'a':
        optionNumber = RenderingConfiguration::ANTIALIAS;
        if (sscanf(&optionString[1], "%lf", &threshold) != EOF) {
            config.setAntialiasThreshold(threshold);
        }
        break;

    case 'X':
    case 'x':
        optionNumber = RenderingConfiguration::EXITENABLE;
        break;

    case 'L':
    case 'l':
        if (config.getFileLocator().searchPaths().size() >= 10) {
            Logger::reportMessage("CommandLineOptions", Logger::FATAL_ERROR, "", "Too many library directories specified\n");
        }
        config.getFileLocator().addSearchPath(&optionString[1]);
        optionNumber = 0;
        break;
    case 'T':
    case 't':
        switch (toupper(optionString[1])) {
        case 'Y':
            config.setTokenizerCaseSensitiveMode(0);
            break;
        case 'N':
            config.setTokenizerCaseSensitiveMode(1);
            break;
        case 'O':
            config.setTokenizerCaseSensitiveMode(2);
            break;
        default:
            config.setTokenizerCaseSensitiveMode(2);
            break;
        }
        optionNumber = 0;
        break;

    case 'S':
    case 's':
        {
            int firstLine;
            sscanf(&optionString[1], "%d", &firstLine);
            config.setFirstLine(firstLine);
        }
        optionNumber = 0;
        break;

    case 'E':
    case 'e':
        {
            int lastLine;
            sscanf(&optionString[1], "%d", &lastLine);
            config.setLastLine(lastLine);
        }
        optionNumber = 0;
        break;

    case 'M': // Switch used so other max values can be inserted easily
    case 'm':
        switch (optionString[1]) {
        case 's': // Max Symbols
        case 'S': {
            int maxSymbols = config.getTokenizerMaxSymbols();
            sscanf(&optionString[2], "%d", &maxSymbols);
            config.setTokenizerMaxSymbols(maxSymbols);
            optionNumber = 0;
            break;
        }
        default:
            break;
        }
        break;

    case 'Q':
    case 'q':
        {
            int quality;
            sscanf(&optionString[1], "%d", &quality);
            config.setQuality(quality);
        }
        optionNumber = 0;
        break;

        // Turn on debugging print statements
    case 'Z':
    case 'z':
        optionNumber = RenderingConfiguration::DEBUGGING;
        break;

    default:
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "\nInvalid option: %s\n\n", --optionString);
            Logger::reportMessage("CommandLineOptions", Logger::ERROR, "", _logMsg);
        }
        optionNumber = 0;
    }

    if (optionNumber != 0) {
        config.setOptionEnabled(optionNumber, addOption);
    }
}

void
CommandLineOptions::parseFileName(const char *fileName, RenderingConfiguration &config,
    Scene &scene, int &numberOfFiles, bool &inFlag, bool &outFlag)
{
    FILE *defaultsFile;
    char optionString[256];

    if (inFlag) // File names may now be separated by spaces from cmdline option
    {
        config.setInputFileName(fileName);
        inFlag = false;
        return;
    }

    if (outFlag) // File names may now be separated by spaces from cmdline option
    {
        config.setOutputFileName(fileName);
        outFlag = false;
        return;
    }

    if (++numberOfFiles > MAX_FILE_NAMES) {
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "\nOnly %d option file names are allowed in a command line.", MAX_FILE_NAMES);
            Logger::reportMessage("CommandLineOptions", Logger::FATAL_ERROR, "", _logMsg);
        }
    }

    if ((defaultsFile = config.getFileLocator().locate(fileName, "r")) != nullptr) {
        while (fgets(optionString, 256, defaultsFile) != nullptr) {
            readOptions(optionString, config, scene, inFlag, outFlag);
        }
        fclose(defaultsFile);
    } else {
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "\nError opening option file %s.", fileName);
            Logger::reportMessage("CommandLineOptions", Logger::ERROR, "", _logMsg);
        }
    }
}

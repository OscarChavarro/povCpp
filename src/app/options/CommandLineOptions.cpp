#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <strings.h>
#include <unistd.h>

#include "vsdk/toolkit/common/logging/Logger.h"
#include "environment/material/PovRayRendererConfiguration.h"
#include "environment/scene/Scene.h"
#include "io/binaryIo/FileLocator.h"
#include "app/options/CommandLineOptions.h"

static constexpr int MAX_FILE_NAMES = 1;

static int
detectProcessorCount()
{
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return (n > 0) ? (int)n : 1;
}

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
    Logger::reportMessage("CommandLineOptions", Logger::WARNING, "", "\n     qflags<letters> = toggle individual quality features (+/- sign"                     " selects on/off): L=lighting S=shadows T=textures F=filtered"                     " shadows R=refraction B=bump M=mirror, e.g. \"+q9 -qflagsS\"");
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
CommandLineOptions::loadDefaults(PovRayRendererConfiguration &config, FileLocator &fileLocator,
    Scene &scene)
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
    defined in config.h.
    */
    // Set Diskwrite as default
    config.setOptionFlags(PovRayRendererConfiguration::DISK_WRITE);
    config.setOutputFormat(PovRayRendererConfiguration::DEFAULT_OUTPUT_FORMAT);

    if ((Option_String_Ptr = getenv("POVRAYOPT")) != nullptr) {
        bool inFlag = false;
        bool outFlag = false;
        readOptions(Option_String_Ptr, config, fileLocator, scene, inFlag, outFlag);
    }
    if ((defaultsFile = fileLocator.locate("povray.def", "r")) != nullptr) {
        bool inFlag = false;
        bool outFlag = false;
        while (fgets(optionString, 256, defaultsFile) != nullptr) {
            readOptions(optionString, config, fileLocator, scene, inFlag, outFlag);
        }
        fclose(defaultsFile);
    }
}

void
CommandLineOptions::parseArguments(int argc, char *argv[], PovRayRendererConfiguration &config,
    FileLocator &fileLocator, Scene &scene)
{
    int numberOfFiles = 0;
    bool inFlag = false;
    bool outFlag = false;
    for (int i = 1; i < argc; i++) {
        if ((*argv[i] == '+') || (*argv[i] == '-')) {
            parseOption(argv[i], config, fileLocator, scene, inFlag, outFlag);
        } else {
            parseFileName(argv[i], config, fileLocator, scene, numberOfFiles, inFlag, outFlag);
        }
    }
}

void
CommandLineOptions::readOptions(const char *optionLine, PovRayRendererConfiguration &config,
    FileLocator &fileLocator, Scene &scene, bool &inFlag, bool &outFlag)
{
    int c;
    short optionLineIndex = 0;
    char optionString[80];

    int stringIndex = 0;
    bool optionStarted = false;
    while ((c = optionLine[optionLineIndex++]) != '\0') {
        if (optionStarted) {
            if (isspace(c)) {
                optionString[stringIndex] = '\0';
                parseOption(optionString, config, fileLocator, scene, inFlag, outFlag);
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
        parseOption(optionString, config, fileLocator, scene, inFlag, outFlag);
    }
}

// Parse the command line parameters
void
CommandLineOptions::parseOption(const char *optionString, PovRayRendererConfiguration &config,
    FileLocator &fileLocator, Scene &scene, bool &inFlag, bool &outFlag)
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

    // Multi-character option: parallel rendering. Both "-parallel" and
    // "+parallel" enable it (project convention: the gate scripts invoke
    // "-parallel" expecting it to turn parallel mode ON), optionally
    // followed by a thread count ("-parallel4"); "...0" forces serial.
    // Checked before the single-letter switch so it never collides with
    // "-p" (PROMPT_EXIT).
    if (strncasecmp(optionString, "parallel", 8) == 0) {
        int n = 0;
        if (sscanf(optionString + 8, "%d", &n) != 1 || n < 0) {
            n = detectProcessorCount();
        }
        if (n == 0) {
            n = 1;
        }
        config.setNumberOfThreads(n);
        config.setOptionEnabled(PovRayRendererConfiguration::PARALLEL, n > 1);
        return;
    }

    // Multi-character option: select the Roth (1982) ray-segment CSG
    // classification algorithm instead of the default point-membership one.
    // Both "-csgRoth" and "+csgRoth" enable it (same convention as
    // "parallel" above: the gate scripts invoke "-csgRoth" expecting it to
    // turn the algorithm ON; there is no use case for disabling it via a
    // sign, since it already defaults to off). Checked before the
    // single-letter switch so it never collides with "-c" (CONTINUE_TRACE).
    if (strncasecmp(optionString, "csgRoth", 7) == 0) {
        config.setOptionEnabled(PovRayRendererConfiguration::CSG_ROTH, true);
        return;
    }

    // Multi-character option: direct feature-flag toggle, independent of the
    // +qN preset (doc/vitralNormalizationAnalysis.md §7.2). Each letter in
    // the spec selects one of the bits setQuality() otherwise sets in bulk;
    // the option's own +/- sign (already parsed into addOption above) decides
    // whether the listed flags are enabled or disabled, so e.g. "+q9 -qflagsS"
    // is the full preset minus shadows, and "+q1 +qflagsT" is the preview plus
    // textures - combinations no single +qN can express. Checked before the
    // single-letter switch so "+qflagsB" never reaches 'q' (quality) or 'f'
    // (output format). Letters: L=surface lighting, S=shadows, T=textures,
    // F=filtered/coloured shadows, R=refraction, B=bump mapping, M=mirror
    // reflection.
    if (strncasecmp(optionString, "qflags", 6) == 0) {
        unsigned int mask = 0;
        for (const char *spec = optionString + 6; *spec != '\0'; spec++) {
            switch (toupper(*spec)) {
            case 'L': mask |= PovRayRendererConfiguration::WITH_SURFACE_LIGHTING; break;
            case 'S': mask |= PovRayRendererConfiguration::WITH_SHADOWS; break;
            case 'T': mask |= PovRayRendererConfiguration::WITH_TEXTURES; break;
            case 'F': mask |= PovRayRendererConfiguration::WITH_FILTERED_SHADOWS; break;
            case 'R': mask |= PovRayRendererConfiguration::WITH_REFRACTION; break;
            case 'B': mask |= PovRayRendererConfiguration::WITH_BUMP_MAPPING; break;
            case 'M': mask |= PovRayRendererConfiguration::WITH_REFLECTION; break;
            default:
                {
                    char _logMsg[256];
                    snprintf(_logMsg, sizeof(_logMsg), "\nUnknown +qflags letter: %c\n", *spec);
                    Logger::reportMessage("CommandLineOptions", Logger::ERROR, "", _logMsg);
                }
                break;
            }
        }
        if (mask != 0) {
            config.setOptionEnabled(mask, addOption);
        }
        return;
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
        optionNumber = PovRayRendererConfiguration::CONTINUE_TRACE;
        break;

    case 'D':
    case 'd':
        optionNumber = PovRayRendererConfiguration::DISPLAY;
        break;

    case '@':
        optionNumber = PovRayRendererConfiguration::VERBOSE_FILE;
        if (optionString[1] == '\0') {
            config.setStatFileName("POVSTAT.OUT");
        } else {
            config.setStatFileName(&optionString[1]);
        }
        break;
    case 'V':
    case 'v':
        optionNumber = PovRayRendererConfiguration::VERBOSE;
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
        optionNumber = PovRayRendererConfiguration::DISK_WRITE;
        if (isupper(optionString[1])) {
            config.setOutputFormat((char)tolower(optionString[1]));
        } else {
            config.setOutputFormat(optionString[1]);
        }

        // Default the output format to the default in the config file
        if (config.getOutputFormat() == '\0') {
            config.setOutputFormat(PovRayRendererConfiguration::DEFAULT_OUTPUT_FORMAT);
        }
        break;

    case 'P':
    case 'p':
        optionNumber = PovRayRendererConfiguration::PROMPT_EXIT;
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
        optionNumber = PovRayRendererConfiguration::ANTIALIAS;
        if (sscanf(&optionString[1], "%lf", &threshold) != EOF) {
            config.setAntialiasThreshold(threshold);
        }
        break;

    case 'X':
    case 'x':
        optionNumber = PovRayRendererConfiguration::EXIT_ENABLE;
        break;

    case 'L':
    case 'l':
        if (fileLocator.searchPaths().size() >= 10) {
            Logger::reportMessage("CommandLineOptions", Logger::FATAL_ERROR, "", "Too many library directories specified\n");
        }
        fileLocator.addSearchPath(&optionString[1]);
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
CommandLineOptions::parseFileName(const char *fileName, PovRayRendererConfiguration &config,
    FileLocator &fileLocator, Scene &scene, int &numberOfFiles, bool &inFlag, bool &outFlag)
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

    if ((defaultsFile = fileLocator.locate(fileName, "r")) != nullptr) {
        while (fgets(optionString, 256, defaultsFile) != nullptr) {
            readOptions(optionString, config, fileLocator, scene, inFlag, outFlag);
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

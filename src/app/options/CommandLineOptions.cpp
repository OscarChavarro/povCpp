#include <cctype>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include "common/logger/Logger.h"
#include "environment/material/RendererConfiguration.h"
#include "io/binaryIo/FileLocator.h"
#include "io/pov/lexer/Tokenizer.h"
#include "render/RenderEngine.h"

#include "app/options/CommandLineOptions.h"

static constexpr int MAX_FILE_NAMES = 1;

int CommandLineOptions::numberOfFiles;
bool CommandLineOptions::inFlag;
bool CommandLineOptions::outFlag;

void
CommandLineOptions::reset()
{
    numberOfFiles = 0;
    inFlag = false;
    outFlag = false;
}

/* Print out usage error message */
void
CommandLineOptions::usage()
{
    Logger::info( "\nUsage:");
    Logger::info( "\n    povray  [+/-] Option1 [+/-] Option2 ...");
    Logger::info( "\n");
    Logger::info( "\n Options: ");
    Logger::info( "\n     dxy = display in format x, using palette option y");
    Logger::info( "\n     vx  = verbose in format x");
    /*Logger::info("\n     @filename  = verbose to file name -- see docs");*/
    Logger::info( "\n     p  = pause before exit");
    Logger::info( "\n     x  = enable early exit by key hit");
    Logger::info( "\n     fx = write output file in format x");
    Logger::info( "\n            ft - Uncompressed Targa-24  fd - DKB/QRT "
                    "Dump  fr - 3 Raw Files");
    Logger::info( "\n     a  = perform antialiasing");
    Logger::info( "\n     c  = continue aborted trace");
    Logger::info( "\n     qx = image quality 0=rough, 9=full");
    Logger::info( "\n     l<pathname> = library path prefix");
    Logger::info( "\n     wxxx = width of the screen");
    Logger::info( "\n     hxxx = height of the screen");
    Logger::info( "\n     sxxx = start at line number xxx");
    Logger::info( "\n     exxx = end at line number xxx");
    Logger::info( "\n     bxxx = Use xxx kilobytes for output file buffer space");
    Logger::info( "\n     i<filename> = input file name");
    Logger::info( "\n     o<filename> = output file name");
    Logger::info( "\n  Ex: +l\\povray\\include +iscene.pov +oscene.tga +w320 "
                    "+h200 +d -v +x");
    Logger::info( "\n  Ex: +iscene.pov +oscene.tga +w160 +h200 +v -d +x");
    Logger::info( "\n");
    exit(1);
}

/* Read the default parameters from povray.def */
void
CommandLineOptions::loadDefaults()
{
    FILE *defaultsFile;
    char optionString[256];
    char *Option_String_Ptr;
    /* READ_ENV_VAR_? should be defined in config.h */
    /* Only one READ_ENV_VAR_? should ever be defined. */
    /* This allows some machines to read environment variable before */
    /* reading povray.def and others to do it after depending on the */
    /* operating system. IBM-PC is before. Default is after if not */
    /* defined in config.h. CDW 2/92 */
    /* Set Diskwrite as default */
    RenderingConfiguration::global().options |= RenderingConfiguration::DISKWRITE;
    RenderingConfiguration::global().outputFormat = RenderingConfiguration::DEFAULT_OUTPUT_FORMAT;

    if ((Option_String_Ptr = getenv("POVRAYOPT")) != nullptr) {
        readOptions(Option_String_Ptr);
    }
    if ((defaultsFile = FileLocator::locate("povray.def", "r")) != nullptr) {
        while (fgets(optionString, 256, defaultsFile) != nullptr) {
            readOptions(optionString);
        }
        fclose(defaultsFile);
    }
}

void
CommandLineOptions::parseArguments(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        if ((*argv[i] == '+') || (*argv[i] == '-')) {
            parseOption(argv[i]);
        } else {
            parseFileName(argv[i]);
        }
    }
}

void
CommandLineOptions::readOptions(char *optionLine)
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
                parseOption(optionString);
                optionStarted = false;
                stringIndex = 0;
            } else {
                optionString[stringIndex++] = (char)c;
            }

        } else /* Option_Started */
            if ((c == (int)'-') || (c == (int)'+')) {
                stringIndex = 0;
                optionString[stringIndex++] = (char)c;
                optionStarted = true;
            } else if (!isspace(c)) {
                Logger::error(
                    "\nBad default file format.  Offending char: (%c), val: "
                    "%d.\n",
                    (char)c, c);
                exit(1);
            }
    }

    if (optionStarted) {
        optionString[stringIndex] = '\0';
        parseOption(optionString);
    }
}

/* parse the command line parameters */
void
CommandLineOptions::parseOption(char *optionString)
{
    bool addOption;
    unsigned int optionNumber = 0;
    double threshold;

    inFlag = outFlag = false; /* if these flags aren't immediately used, reset
                                 them on next -/+ option! */
    if (*(optionString++) == '-') {
        addOption = false;
    } else {
        addOption = true;
    }

    switch (*optionString) {
    case 'B':
    case 'b':
        sscanf(&optionString[1], "%d", &RenderingConfiguration::global().fileBufferSize);
        RenderingConfiguration::global().fileBufferSize *= 1024;
        if (RenderingConfiguration::global().fileBufferSize < BUFSIZ) {
            RenderingConfiguration::global().fileBufferSize = BUFSIZ;
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
        RenderingConfiguration::global().displayFormat = '0';
        RenderingConfiguration::global().paletteOption = '3';
        if (optionString[1] != '\0') {
            RenderingConfiguration::global().displayFormat = (char)toupper(optionString[1]);
        }

        if (optionString[1] != '\0' && optionString[2] != '\0') {
            RenderingConfiguration::global().paletteOption = (char)toupper(optionString[2]);
        }
        break;

    case '@':
        optionNumber = RenderingConfiguration::VERBOSE_FILE;
        if (optionString[1] == '\0') {
            strcpy(RenderingConfiguration::global().statFileName, "POVSTAT.OUT");
        } else {
            strncpy(RenderingConfiguration::global().statFileName, &optionString[1], RenderingConfiguration::RENDER_FILE_NAME_LENGTH - 1);
            RenderingConfiguration::global().statFileName[RenderingConfiguration::RENDER_FILE_NAME_LENGTH - 1] = '\0';
        }
        break;
    case 'V':
    case 'v':
        optionNumber = RenderingConfiguration::VERBOSE;
        RenderingConfiguration::global().verboseFormat = (char)toupper(optionString[1]);
        if (RenderingConfiguration::global().verboseFormat == '\0') {
            RenderingConfiguration::global().verboseFormat = '1';
        }
        break;

    case 'W':
    case 'w':
        sscanf(&optionString[1], "%d", &RenderEngine::renderFrame().screenWidth);
        optionNumber = 0;
        break;

    case 'H':
    case 'h':
        sscanf(&optionString[1], "%d", &RenderEngine::renderFrame().screenHeight);
        optionNumber = 0;
        break;

    case 'F':
    case 'f':
        optionNumber = RenderingConfiguration::DISKWRITE;
        if (isupper(optionString[1])) {
            RenderingConfiguration::global().outputFormat = (char)tolower(optionString[1]);
        } else {
            RenderingConfiguration::global().outputFormat = optionString[1];
        }

        /* Default the output format to the default in the config file */
        if (RenderingConfiguration::global().outputFormat == '\0') {
            RenderingConfiguration::global().outputFormat = RenderingConfiguration::DEFAULT_OUTPUT_FORMAT;
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
            strncpy(RenderingConfiguration::global().inputFileName, &optionString[1], RenderingConfiguration::RENDER_FILE_NAME_LENGTH - 1);
            RenderingConfiguration::global().inputFileName[RenderingConfiguration::RENDER_FILE_NAME_LENGTH - 1] = '\0';
        }
        optionNumber = 0;
        break;

    case 'O':
    case 'o':
        if (optionString[1] == '\0') {
            outFlag = true;
        } else {
            strncpy(RenderingConfiguration::global().outputFileName, &optionString[1], RenderingConfiguration::RENDER_FILE_NAME_LENGTH - 1);
            RenderingConfiguration::global().outputFileName[RenderingConfiguration::RENDER_FILE_NAME_LENGTH - 1] = '\0';
        }
        optionNumber = 0;
        break;

    case 'A':
    case 'a':
        optionNumber = RenderingConfiguration::ANTIALIAS;
        if (sscanf(&optionString[1], "%lf", &threshold) != EOF) {
            RenderingConfiguration::global().antialiasThreshold = threshold;
        }
        break;

    case 'X':
    case 'x':
        optionNumber = RenderingConfiguration::EXITENABLE;
        break;

    case 'L':
    case 'l':
        if (FileLocator::searchPaths().size() >= 10) {
            Logger::error("Too many library directories specified\n");
            exit(1);
        }
        FileLocator::addSearchPath(&optionString[1]);
        optionNumber = 0;
        break;
    case 'T':
    case 't':
        switch (toupper(optionString[1])) {
        case 'Y':
            Tokenizer::setCaseSensitiveIdentifiers(0);
            break;
        case 'N':
            Tokenizer::setCaseSensitiveIdentifiers(1);
            break;
        case 'O':
            Tokenizer::setCaseSensitiveIdentifiers(2);
            break;
        default:
            Tokenizer::setCaseSensitiveIdentifiers(2);
            break;
        }
        optionNumber = 0;
        break;

    case 'S':
    case 's':
        sscanf(&optionString[1], "%d", &RenderingConfiguration::global().firstLine);
        optionNumber = 0;
        break;

    case 'E':
    case 'e':
        sscanf(&optionString[1], "%d", &RenderingConfiguration::global().lastLine);
        optionNumber = 0;
        break;

    case 'M': /* Switch used so other max values can be inserted easily */
    case 'm':
        switch (optionString[1]) {
        case 's': /* Max Symbols */
        case 'S': {
            int maxSymbols = Tokenizer::getMaxSymbols();
            sscanf(&optionString[2], "%d", &maxSymbols);
            Tokenizer::setMaxSymbols(maxSymbols);
            optionNumber = 0;
            break;
        }
        default:
            break;
        }
        break;

    case 'Q':
    case 'q':
        sscanf(&optionString[1], "%d", &RenderingConfiguration::global().quality);
        optionNumber = 0;
        break;

        /* Turn on debugging print statements. */
    case 'Z':
    case 'z':
        optionNumber = RenderingConfiguration::DEBUGGING;
        break;

    default:
        Logger::error("\nInvalid option: %s\n\n", --optionString);
        optionNumber = 0;
    }

    if (optionNumber != 0) {
        if (addOption) {
            RenderingConfiguration::global().options |= optionNumber;
        } else {
            RenderingConfiguration::global().options &= ~optionNumber;
        }
    }
}

void
CommandLineOptions::printOptions()
{
    Logger::info( "\nPOV-Ray          Options in effect: ");

    if (RenderingConfiguration::global().options & RenderingConfiguration::CONTINUE_TRACE) {
        Logger::info( "+c ");
    }

    if (RenderingConfiguration::global().options & RenderingConfiguration::DISPLAY) {
        Logger::info( "+d%c%c ", RenderingConfiguration::global().displayFormat, RenderingConfiguration::global().paletteOption);
    }

    if (RenderingConfiguration::global().options & RenderingConfiguration::VERBOSE) {
        Logger::info( "+v%c ", RenderingConfiguration::global().verboseFormat);
    }

    if (RenderingConfiguration::global().options & RenderingConfiguration::VERBOSE_FILE) {
        Logger::info( "+@%s ", RenderingConfiguration::global().statFileName);
    }

    if (RenderingConfiguration::global().options & RenderingConfiguration::DISKWRITE) {
        Logger::info( "+f%c ", RenderingConfiguration::global().outputFormat);
    }

    if (RenderingConfiguration::global().options & RenderingConfiguration::PROMPTEXIT) {
        Logger::info( "+p ");
    }

    if (RenderingConfiguration::global().options & RenderingConfiguration::EXITENABLE) {
        Logger::info( "+x ");
    }

    if (RenderingConfiguration::global().options & RenderingConfiguration::ANTIALIAS) {
        Logger::info( "+a%f ", RenderingConfiguration::global().antialiasThreshold);
    }

    if (RenderingConfiguration::global().options & RenderingConfiguration::DEBUGGING) {
        Logger::info( "+z ");
    }

    if (RenderingConfiguration::global().fileBufferSize != 0) {
        Logger::info( "-b%d ", RenderingConfiguration::global().fileBufferSize / 1024);
    }

    Logger::info( "-q%d -w%d -h%d -s%d -e%d\n-i%s ", RenderingConfiguration::global().quality,
        RenderEngine::renderFrame().screenWidth, RenderEngine::renderFrame().screenHeight, RenderingConfiguration::global().firstLine,
        RenderingConfiguration::global().lastLine, RenderingConfiguration::global().inputFileName);

    if (RenderingConfiguration::global().options & RenderingConfiguration::DISKWRITE) {
        Logger::info( "-o%s ", RenderingConfiguration::global().outputFileName);
    }

    const java::ArrayList<java::String> &paths = FileLocator::searchPaths();
    for (long int i = 0; i < paths.size(); i++) {
        Logger::info("-l%s ", paths.get(i).toCString());
    }

    Logger::info( "\n");
}

void
CommandLineOptions::parseFileName(char *fileName)
{
    FILE *defaultsFile;
    char optionString[256];

    if (inFlag) /* file names may now be separated by spaces from cmdline option
                 */
    {
        strncpy(RenderingConfiguration::global().inputFileName, fileName, RenderingConfiguration::RENDER_FILE_NAME_LENGTH - 1);
        RenderingConfiguration::global().inputFileName[RenderingConfiguration::RENDER_FILE_NAME_LENGTH - 1] = '\0';
        inFlag = false;
        return;
    }

    if (outFlag) /* file names may now be separated by spaces from cmdline
                    option */
    {
        strncpy(RenderingConfiguration::global().outputFileName, fileName, RenderingConfiguration::RENDER_FILE_NAME_LENGTH - 1);
        RenderingConfiguration::global().outputFileName[RenderingConfiguration::RENDER_FILE_NAME_LENGTH - 1] = '\0';
        outFlag = false;
        return;
    }

    if (++numberOfFiles > MAX_FILE_NAMES) {
        Logger::error(
            "\nOnly %d option file names are allowed in a command line.",
            MAX_FILE_NAMES);
        exit(1);
    }

    if ((defaultsFile = FileLocator::locate(fileName, "r")) != nullptr) {
        while (fgets(optionString, 256, defaultsFile) != nullptr) {
            readOptions(optionString);
        }
        fclose(defaultsFile);
    } else {
        Logger::error("\nError opening option file %s.", fileName);
    }
}

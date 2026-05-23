/****************************************************************************
 *                     povray.c
 *
 *  This module contains the entry routine for the raytracer and the code to
 *  parse the parameters on the command line.
 *
 *****************************************************************************/
#include "app/PovrayApplication.h"
#include "common/dataStructures/PriorityQueue.h"
#include "common/logger/Logger.h"
#include "environment/geometry/Intersection.h"
#include "java/io/FileInputStream.h"
#include "io/FileLocator.h"
#include "io/Parse.h"
#include "io/image/DumpFormat.h"
#include "io/image/RawFormat.h"
#include "io/image/TargaFormat.h"
#include "render/RenderEngine.h"
#include "environment/scene/SceneFrame.h"
#include "environment/material/RendererConfiguration.h"
#include "common/Statistics.h"
#include <cctype>
#include <cstring>
#include <ctime> /* BP */

static constexpr int MAX_FILE_NAMES = 1;
static constexpr const char *COMPILER_VER = ".u";

FILE *bfp;

extern RenderFrame globalFrame;

int maxSymbols = 500;

static int numberOfFiles;
static int inFlag;
static int outFlag;
double VTemp;

int
main(int argc, char *argv[])
{
    PovrayApplication::run(argc, argv);

    return 0;
}

void
PovrayApplication::run(int argc, char *argv[])
{
    initializeFromCommandLine(argc, argv);
    configureOutputTarget();
    parseSceneDescription();
    prepareRendering();
    runRenderLoop();
    finalizeRun();
}

void
PovrayApplication::initializeFromCommandLine(int argc, char *argv[])
{
    int i;

    if (argc == 1) {
        usage();
    }

    initVars();
    FileLocator::clearSearchPaths();

    getDefaults();

    for (i = 1; i < argc; i++) {
        if ((*argv[i] == '+') || (*argv[i] == '-')) {
            parseOption(argv[i]);
        } else {
            parseFileName(argv[i]);
        }
    }

    if (globalRenderingConfiguration.lastLine == -1) {
        globalRenderingConfiguration.lastLine = globalFrame.Screen_Height;
    }
}

void
PovrayApplication::configureOutputTarget()
{
    if (!(globalRenderingConfiguration.options & DISKWRITE)) {
        return;
    }

    switch (globalRenderingConfiguration.outputFormat) {
    case '\0':
    case 'd':
    case 'D':
        if ((globalRenderingConfiguration.outputFileInputStream = DumpFormat::getDumpFileInputStream()) ==
            nullptr) {
            closeAll();
            exit(1);
        }
        break;
    case 'r':
    case 'R':
        if ((globalRenderingConfiguration.outputFileInputStream = RawFormat::getRawFileInputStream()) ==
            nullptr) {
            closeAll();
            exit(1);
        }
        break;
    case 't':
    case 'T':
        if ((globalRenderingConfiguration.outputFileInputStream = TargaFormat::getTargaFileInputStream()) ==
            nullptr) {
            closeAll();
            exit(1);
        }
        break;
    default:
        Logger::error("Unrecognized output file format %c\n", globalRenderingConfiguration.outputFormat);
        exit(1);
    }

    if (globalRenderingConfiguration.outputFileName[0] == '\0') {
        strcpy(globalRenderingConfiguration.outputFileName,
            globalRenderingConfiguration.outputFileInputStream->defaultFileName());
    }
}

void
PovrayApplication::parseSceneDescription()
{
    FILE *statFile;

    printOptions();

    Tokenizer::initializeTokenizer(globalRenderingConfiguration.inputFileName);
    fprintf(stderr, "Parsing...");
    if (globalRenderingConfiguration.options & VERBOSE_FILE) {
        statFile = fopen(globalRenderingConfiguration.statFileName, "w+t");
        fprintf(statFile, "Parsing...\n");
        fclose(statFile);
    }

    SceneParser::Parse(&globalFrame);
    Tokenizer::terminateTokenizer();
}

void
PovrayApplication::prepareRendering()
{
    if (globalRenderingConfiguration.options & DISPLAY) {
        printf("Displaying...\n");
    }

    if (globalRenderingConfiguration.options & DISKWRITE) {
        if (globalRenderingConfiguration.options & CONTINUE_TRACE) {
            if (globalRenderingConfiguration.outputFileInputStream->open(globalRenderingConfiguration.outputFileName,
                    &globalFrame.Screen_Width, &globalFrame.Screen_Height,
                    globalRenderingConfiguration.fileBufferSize, FileInputStream::READ_MODE) != 1) {
                Logger::error("Error opening continue trace output file\n");
                fprintf(
                    stderr, "Opening new output file %s.\n", globalRenderingConfiguration.outputFileName);
                globalRenderingConfiguration.options &= ~CONTINUE_TRACE;

                if (globalRenderingConfiguration.outputFileInputStream->open(globalRenderingConfiguration.outputFileName,
                        &globalFrame.Screen_Width, &globalFrame.Screen_Height,
                        globalRenderingConfiguration.fileBufferSize, FileInputStream::WRITE_MODE) != 1) {
                    Logger::error("Error opening output file\n");
                    closeAll();
                    exit(1);
                }
            }

            RenderEngine::initializeRenderer();
            if (globalRenderingConfiguration.options & CONTINUE_TRACE) {
                RenderEngine::readRenderedPart();
            }
        } else {
            if (globalRenderingConfiguration.outputFileInputStream->open(globalRenderingConfiguration.outputFileName,
                    &globalFrame.Screen_Width, &globalFrame.Screen_Height,
                    globalRenderingConfiguration.fileBufferSize, FileInputStream::WRITE_MODE) != 1) {
                Logger::error("Error opening output file\n");
                closeAll();
                exit(1);
            }

            RenderEngine::initializeRenderer();
        }
    } else {
        RenderEngine::initializeRenderer();
    }

    IntersectionPriorityQueuePool::pqInit();
    TextureUtils::initializeNoise();
}

void
PovrayApplication::runRenderLoop()
{
    FILE *statFile;

    globalStatistics.startTimer();

    if ((globalRenderingConfiguration.options & VERBOSE) && (globalRenderingConfiguration.verboseFormat != '1')) {
        printf("Rendering...\n");
    } else if ((globalRenderingConfiguration.options & VERBOSE) && (globalRenderingConfiguration.verboseFormat == '1')) {
        fprintf(stderr, "POV-Ray rendering %s to %s :\n", globalRenderingConfiguration.inputFileName,
            globalRenderingConfiguration.outputFileName);
    }
    if (globalRenderingConfiguration.options & VERBOSE_FILE) {
        statFile = fopen(globalRenderingConfiguration.statFileName, "w+t");
        fprintf(statFile, "Parsed ok. Now rendering %s to %s :\n",
            globalRenderingConfiguration.inputFileName, globalRenderingConfiguration.outputFileName);
        fclose(statFile);
    }

    RenderEngine::startTracing();

    if (globalRenderingConfiguration.options & VERBOSE && globalRenderingConfiguration.verboseFormat == '1') {
        fprintf(stderr, "\n");
    }
}

void
PovrayApplication::finalizeRun()
{
    FILE *statFile;

    globalStatistics.stopTimer();

    closeAll();
    globalStatistics.print(globalFrame, globalRenderingConfiguration);

    if (globalRenderingConfiguration.options & VERBOSE_FILE) {
        statFile = fopen(globalRenderingConfiguration.statFileName, "a+t");
        fprintf(statFile, "Done Tracing\n");
        fclose(statFile);
    }
}

/* Print out usage error message */
void
PovrayApplication::usage()
{
    fprintf(stdout, "\nUsage:");
    fprintf(stdout, "\n    povray  [+/-] Option1 [+/-] Option2 ...");
    fprintf(stdout, "\n");
    fprintf(stdout, "\n Options: ");
    fprintf(stdout, "\n     dxy = display in format x, using palette option y");
    fprintf(stdout, "\n     vx  = verbose in format x");
    /*fprintf (stdout,"\n     @filename  = verbose to file name -- see docs");*/
    fprintf(stdout, "\n     p  = pause before exit");
    fprintf(stdout, "\n     x  = enable early exit by key hit");
    fprintf(stdout, "\n     fx = write output file in format x");
    fprintf(stdout, "\n            ft - Uncompressed Targa-24  fd - DKB/QRT "
                    "Dump  fr - 3 Raw Files");
    fprintf(stdout, "\n     a  = perform antialiasing");
    fprintf(stdout, "\n     c  = continue aborted trace");
    fprintf(stdout, "\n     qx = image quality 0=rough, 9=full");
    fprintf(stdout, "\n     l<pathname> = library path prefix");
    fprintf(stdout, "\n     wxxx = width of the screen");
    fprintf(stdout, "\n     hxxx = height of the screen");
    fprintf(stdout, "\n     sxxx = start at line number xxx");
    fprintf(stdout, "\n     exxx = end at line number xxx");
    fprintf(
        stdout, "\n     bxxx = Use xxx kilobytes for output file buffer space");
    fprintf(stdout, "\n     i<filename> = input file name");
    fprintf(stdout, "\n     o<filename> = output file name");
    fprintf(stdout, "\n  Ex: +l\\povray\\include +iscene.pov +oscene.tga +w320 "
                    "+h200 +d -v +x");
    fprintf(stdout, "\n  Ex: +iscene.pov +oscene.tga +w160 +h200 +v -d +x");
    fprintf(stdout, "\n");
    exit(1);
}

void
PovrayApplication::initVars()
{
    globalRenderingConfiguration.reset();
    globalStatistics.reset();
    Tokenizer::setCaseSensitiveIdentifiers(0);
    numberOfFiles = 0;

    globalFrame.Screen_Height = 100;
    globalFrame.Screen_Width = 100;
}

/* Close all the stuff that has been opened. */
void
PovrayApplication::closeAll()
{
    if (globalRenderingConfiguration.outputFileInputStream) {
        globalRenderingConfiguration.outputFileInputStream->close();
    }
}

/* Read the default parameters from povray.def */
void
PovrayApplication::getDefaults()
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
    globalRenderingConfiguration.options |= DISKWRITE;
    globalRenderingConfiguration.outputFormat = DEFAULT_OUTPUT_FORMAT;

    if ((Option_String_Ptr = getenv("POVRAYOPT")) != nullptr) {
        PovrayApplication::readOptions(Option_String_Ptr);
    }
    if ((defaultsFile = FileLocator::locate("povray.def", "r")) != nullptr) {
        while (fgets(optionString, 256, defaultsFile) != nullptr) {
            PovrayApplication::readOptions(optionString);
        }
        fclose(defaultsFile);
    }
}

void
PovrayApplication::readOptions(char *optionLine)
{
    int c;
    int stringIndex;
    int optionStarted;
    short optionLineIndex = 0;
    char optionString[80];

    stringIndex = 0;
    optionStarted = FALSE;
    while ((c = optionLine[optionLineIndex++]) != '\0') {
        if (optionStarted) {
            if (isspace(c)) {
                optionString[stringIndex] = '\0';
                PovrayApplication::parseOption(optionString);
                optionStarted = FALSE;
                stringIndex = 0;
            } else {
                optionString[stringIndex++] = (char)c;
            }

        } else /* Option_Started */
            if ((c == (int)'-') || (c == (int)'+')) {
                stringIndex = 0;
                optionString[stringIndex++] = (char)c;
                optionStarted = TRUE;
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
        PovrayApplication::parseOption(optionString);
    }
}

/* parse the command line parameters */
void
PovrayApplication::parseOption(char *optionString)
{
    int addOption;
    unsigned int optionNumber = 0;
    double threshold;

    inFlag = outFlag = FALSE; /* if these flags aren't immediately used, reset
                                 them on next -/+ option! */
    if (*(optionString++) == '-') {
        addOption = FALSE;
    } else {
        addOption = TRUE;
    }

    switch (*optionString) {
    case 'B':
    case 'b':
        sscanf(&optionString[1], "%d", &globalRenderingConfiguration.fileBufferSize);
        globalRenderingConfiguration.fileBufferSize *= 1024;
        if (globalRenderingConfiguration.fileBufferSize < BUFSIZ) {
            globalRenderingConfiguration.fileBufferSize = BUFSIZ;
        }
        optionNumber = 0;
        break;

    case 'C':
    case 'c':
        optionNumber = CONTINUE_TRACE;
        break;

    case 'D':
    case 'd':
        optionNumber = DISPLAY;
        globalRenderingConfiguration.displayFormat = '0';
        globalRenderingConfiguration.paletteOption = '3';
        if (optionString[1] != '\0') {
            globalRenderingConfiguration.displayFormat = (char)toupper(optionString[1]);
        }

        if (optionString[1] != '\0' && optionString[2] != '\0') {
            globalRenderingConfiguration.paletteOption = (char)toupper(optionString[2]);
        }
        break;

    case '@':
        optionNumber = VERBOSE_FILE;
        if (optionString[1] == '\0') {
            strcpy(globalRenderingConfiguration.statFileName, "POVSTAT.OUT");
        } else {
            strncpy(globalRenderingConfiguration.statFileName, &optionString[1], FILE_NAME_LENGTH - 1);
            globalRenderingConfiguration.statFileName[FILE_NAME_LENGTH - 1] = '\0';
        }
        break;
    case 'V':
    case 'v':
        optionNumber = VERBOSE;
        globalRenderingConfiguration.verboseFormat = (char)toupper(optionString[1]);
        if (globalRenderingConfiguration.verboseFormat == '\0') {
            globalRenderingConfiguration.verboseFormat = '1';
        }
        break;

    case 'W':
    case 'w':
        sscanf(&optionString[1], "%d", &globalFrame.Screen_Width);
        optionNumber = 0;
        break;

    case 'H':
    case 'h':
        sscanf(&optionString[1], "%d", &globalFrame.Screen_Height);
        optionNumber = 0;
        break;

    case 'F':
    case 'f':
        optionNumber = DISKWRITE;
        if (isupper(optionString[1])) {
            globalRenderingConfiguration.outputFormat = (char)tolower(optionString[1]);
        } else {
            globalRenderingConfiguration.outputFormat = optionString[1];
        }

        /* Default the output format to the default in the config file */
        if (globalRenderingConfiguration.outputFormat == '\0') {
            globalRenderingConfiguration.outputFormat = DEFAULT_OUTPUT_FORMAT;
        }
        break;

    case 'P':
    case 'p':
        optionNumber = PROMPTEXIT;
        break;

    case 'I':
    case 'i':
        if (optionString[1] == '\0') {
            inFlag = TRUE;
        } else {
            strncpy(globalRenderingConfiguration.inputFileName, &optionString[1], FILE_NAME_LENGTH - 1);
            globalRenderingConfiguration.inputFileName[FILE_NAME_LENGTH - 1] = '\0';
        }
        optionNumber = 0;
        break;

    case 'O':
    case 'o':
        if (optionString[1] == '\0') {
            outFlag = TRUE;
        } else {
            strncpy(globalRenderingConfiguration.outputFileName, &optionString[1], FILE_NAME_LENGTH - 1);
            globalRenderingConfiguration.outputFileName[FILE_NAME_LENGTH - 1] = '\0';
        }
        optionNumber = 0;
        break;

    case 'A':
    case 'a':
        optionNumber = ANTIALIAS;
        if (sscanf(&optionString[1], "%lf", &threshold) != EOF) {
            globalRenderingConfiguration.antialiasThreshold = threshold;
        }
        break;

    case 'X':
    case 'x':
        optionNumber = EXITENABLE;
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
        sscanf(&optionString[1], "%d", &globalRenderingConfiguration.firstLine);
        optionNumber = 0;
        break;

    case 'E':
    case 'e':
        sscanf(&optionString[1], "%d", &globalRenderingConfiguration.lastLine);
        optionNumber = 0;
        break;

    case 'M': /* Switch used so other max values can be inserted easily */
    case 'm':
        switch (optionString[1]) {
        case 's': /* Max Symbols */
        case 'S':
            sscanf(&optionString[2], "%d", &maxSymbols);
            optionNumber = 0;
            break;
        default:
            break;
        }
        break;

    case 'Q':
    case 'q':
        sscanf(&optionString[1], "%d", &globalRenderingConfiguration.quality);
        optionNumber = 0;
        break;

        /* Turn on debugging print statements. */
    case 'Z':
    case 'z':
        optionNumber = DEBUGGING;
        break;

    default:
        Logger::error("\nInvalid option: %s\n\n", --optionString);
        optionNumber = 0;
    }

    if (optionNumber != 0) {
        if (addOption) {
            globalRenderingConfiguration.options |= optionNumber;
        } else {
            globalRenderingConfiguration.options &= ~optionNumber;
        }
    }
}

void
PovrayApplication::printOptions()
{
    fprintf(stdout, "\nPOV-Ray          Options in effect: ");

    if (globalRenderingConfiguration.options & CONTINUE_TRACE) {
        fprintf(stdout, "+c ");
    }

    if (globalRenderingConfiguration.options & DISPLAY) {
        fprintf(stdout, "+d%c%c ", globalRenderingConfiguration.displayFormat, globalRenderingConfiguration.paletteOption);
    }

    if (globalRenderingConfiguration.options & VERBOSE) {
        fprintf(stdout, "+v%c ", globalRenderingConfiguration.verboseFormat);
    }

    if (globalRenderingConfiguration.options & VERBOSE_FILE) {
        fprintf(stdout, "+@%s ", globalRenderingConfiguration.statFileName);
    }

    if (globalRenderingConfiguration.options & DISKWRITE) {
        fprintf(stdout, "+f%c ", globalRenderingConfiguration.outputFormat);
    }

    if (globalRenderingConfiguration.options & PROMPTEXIT) {
        fprintf(stdout, "+p ");
    }

    if (globalRenderingConfiguration.options & EXITENABLE) {
        fprintf(stdout, "+x ");
    }

    if (globalRenderingConfiguration.options & ANTIALIAS) {
        fprintf(stdout, "+a%f ", globalRenderingConfiguration.antialiasThreshold);
    }

    if (globalRenderingConfiguration.options & DEBUGGING) {
        fprintf(stdout, "+z ");
    }

    if (globalRenderingConfiguration.fileBufferSize != 0) {
        fprintf(stdout, "-b%d ", globalRenderingConfiguration.fileBufferSize / 1024);
    }

    fprintf(stdout, "-q%d -w%d -h%d -s%d -e%d\n-i%s ", globalRenderingConfiguration.quality,
        globalFrame.Screen_Width, globalFrame.Screen_Height, globalRenderingConfiguration.firstLine,
        globalRenderingConfiguration.lastLine, globalRenderingConfiguration.inputFileName);

    if (globalRenderingConfiguration.options & DISKWRITE) {
        fprintf(stdout, "-o%s ", globalRenderingConfiguration.outputFileName);
    }

    for (std::vector<std::string>::const_iterator path =
             FileLocator::searchPaths().begin();
         path != FileLocator::searchPaths().end(); ++path) {
        fprintf(stdout, "-l%s ", path->c_str());
    }

    fprintf(stdout, "\n");
}

void
PovrayApplication::parseFileName(char *fileName)
{
    FILE *defaultsFile;
    char optionString[256];

    if (inFlag) /* file names may now be separated by spaces from cmdline option
                 */
    {
        strncpy(globalRenderingConfiguration.inputFileName, fileName, FILE_NAME_LENGTH - 1);
        globalRenderingConfiguration.inputFileName[FILE_NAME_LENGTH - 1] = '\0';
        inFlag = FALSE;
        return;
    }

    if (outFlag) /* file names may now be separated by spaces from cmdline
                    option */
    {
        strncpy(globalRenderingConfiguration.outputFileName, fileName, FILE_NAME_LENGTH - 1);
        globalRenderingConfiguration.outputFileName[FILE_NAME_LENGTH - 1] = '\0';
        outFlag = FALSE;
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
            PovrayApplication::readOptions(optionString);
        }
        fclose(defaultsFile);
    } else {
        Logger::error("\nError opening option file %s.", fileName);
    }
}

void
PovrayApplication::printCredits()
{
    fprintf(stderr, "\n");
    fprintf(
        stderr, "  Persistence of Vision Raytracer Ver 1.0%s\n", COMPILER_VER);
    fprintf(stderr, "  Copyright 1992 POV-Team\n");
    fprintf(stderr, "  "
                    "----------------------------------------------------------"
                    "------------\n");
    fprintf(stderr, "  POV-Ray is based on DKBTrace 2.12 by David K. Buck & "
                    "Aaron A. Collins.\n");
    fprintf(stderr, "  \n");
    fprintf(stderr, "  Contributing Authors: (Alphabetically)\n");
    fprintf(stderr, "  \n");
    fprintf(stderr,
        "  Steve A. Bennett    David K. Buck        Aaron A. Collins\n");
    fprintf(
        stderr, "  Alexander Enzmann  Dan Farmer            Girish T. Hagan\n");
    fprintf(
        stderr, "  Douglas Muir         Bill Pulver          Robert Skinner\n");
    fprintf(
        stderr, "  Scott Taylor         Drew Wells            Chris Young\n");
    fprintf(stderr, "  "
                    "----------------------------------------------------------"
                    "------------\n");
    fprintf(stderr, "  Other contributors listed in the documentation.\n");
}

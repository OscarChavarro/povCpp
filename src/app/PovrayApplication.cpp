#include <cctype>
#include <cstring>
#include <ctime> /* BP */

#include "io/image/ImageFileHandle.h"

#include "common/dataStructures/PriorityQueue.h"
#include "common/logger/Logger.h"
#include "common/Statistics.h"
#include "environment/geometry/Intersection.h"
#include "environment/scene/SceneFrame.h"
#include "environment/material/RendererConfiguration.h"
#include "io/FileLocator.h"
#include "io/Parse.h"
#include "io/image/DumpFormat.h"
#include "io/image/RawFormat.h"
#include "io/image/TargaFormat.h"
#include "render/RenderEngine.h"

#include "app/PovrayApplication.h"

static constexpr int MAX_FILE_NAMES = 1;
static constexpr const char *COMPILER_VER = ".u";

FILE *bfp;

extern RenderFrame globalFrame;

int maxSymbols = 500;

static int numberOfFiles;
static int inFlag;
static int outFlag;
double VTemp;

static void
printStatistics(
    const Statistics &stats,
    const RenderFrame &frame,
    const RenderingConfiguration &configuration)
{
    FILE *statOut = stdout;
    if (configuration.options & VERBOSE_FILE) {
        statOut = fopen(configuration.statFileName, "w+t");
    }

    const long pixelsInImage =
        (long)frame.Screen_Width * (long)frame.Screen_Height;

    fprintf(statOut, "\n%s statistics\n", configuration.inputFileName);
    if (pixelsInImage > stats.numberOfPixels) {
        fprintf(statOut, "  Partial Image Rendered");
    }

    fprintf(statOut, "--------------------------------------\n");
    fprintf(statOut, "Resolution %d x %d\n", frame.Screen_Width,
        frame.Screen_Height);
    fprintf(statOut,
        "# Rays:  %10ld     # Pixels:  %10ld  # Pixels supersampled: %10ld\n",
        stats.numberOfRays, stats.numberOfPixels, stats.numberOfPixelsSupersampled);

    fprintf(statOut, "  Ray->Shape Intersection Tests:\n");
    fprintf(statOut,
        "    Type                 Tests     Succeeded    Percentage\n");
    fprintf(statOut,
        "  -----------------------------------------------------------\n");
#define PRINT_INTERSECTION_ROW(label, tests, succeeded)                         \
    if (tests) {                                                                 \
        fprintf(statOut, label " %10ld  %10ld  %10.2f\n", tests, succeeded,    \
            (((double)succeeded / (double)tests) * 100.0));                     \
    }
    PRINT_INTERSECTION_ROW("  Sphere        ", stats.raySphereTests, stats.raySphereTestsSucceeded);
    PRINT_INTERSECTION_ROW("  Plane         ", stats.rayPlaneTests, stats.rayPlaneTestsSucceeded);
    PRINT_INTERSECTION_ROW(
        "  Triangle     ", stats.rayTriangleTests, stats.rayTriangleTestsSucceeded);
    PRINT_INTERSECTION_ROW("  Quadric       ", stats.rayQuadricTests, stats.rayQuadricTestsSucceeded);
    PRINT_INTERSECTION_ROW("  Blob          ", stats.rayBlobTests, stats.rayBlobTestsSucceeded);
    PRINT_INTERSECTION_ROW("  Box           ", stats.rayBoxTests, stats.rayBoxTestsSucceeded);
    PRINT_INTERSECTION_ROW("  Quartic\\Poly ", stats.rayPolyTests, stats.rayPolyTestsSucceeded);
    PRINT_INTERSECTION_ROW(
        "  Bezier Patch ", stats.rayBicubicTests, stats.rayBicubicTestsSucceeded);
    PRINT_INTERSECTION_ROW(
        "  Height Fld   ", stats.rayHtFieldTests, stats.rayHtFieldTestsSucceeded);
    PRINT_INTERSECTION_ROW(
        "  Hght Fld Box ", stats.rayHtFieldBoxTests, stats.rayHFieldBoxTestsSucceeded);
    PRINT_INTERSECTION_ROW(
        "  Bounds        ", stats.boundingRegionTests, stats.boundingRegionTestsSucceeded);
    PRINT_INTERSECTION_ROW(
        "  Clips         ", stats.clippingRegionTests, stats.clippingRegionTestsSucceeded);
#undef PRINT_INTERSECTION_ROW

    if (stats.callsToNoise) {
        fprintf(statOut, "  Calls to Noise:    %10ld\n", stats.callsToNoise);
    }
    if (stats.callsToDNoise) {
        fprintf(statOut, "  Calls to DNoise:  %10ld\n", stats.callsToDNoise);
    }
    if (stats.shadowRayTests) {
        fprintf(statOut,
            "  Shadow Ray Tests: %10ld      Blocking Objects Found:  %10ld\n",
            stats.shadowRayTests, stats.shadowRaysSucceeded);
    }
    if (stats.reflectedRaysTraced) {
        fprintf(statOut, "  Reflected Rays:    %10ld\n", stats.reflectedRaysTraced);
    }
    if (stats.refractedRaysTraced) {
        fprintf(statOut, "  Refracted Rays:    %10ld\n", stats.refractedRaysTraced);
    }
    if (stats.transmittedRaysTraced) {
        fprintf(statOut, "  Transmitted Rays: %10ld\n", stats.transmittedRaysTraced);
    }

    if (stats.usedTime != 0.0) {
        const int hours = (int)stats.usedTime / 3600;
        const int minutes = (int)(stats.usedTime - hours * 3600) / 60;
        const double seconds = stats.usedTime - (double)(hours * 3600 + minutes * 60);
        fprintf(statOut,
            "  Time For Trace:    %2d hours %2d minutes %4.2f seconds\n", hours,
            minutes, seconds);
    }
    if (configuration.options & VERBOSE_FILE) {
        fclose(statOut);
    }
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
        Logger::info("Displaying...\n");
    }

    if (globalRenderingConfiguration.options & DISKWRITE) {
        if (globalRenderingConfiguration.options & CONTINUE_TRACE) {
            if (globalRenderingConfiguration.outputFileInputStream->open(globalRenderingConfiguration.outputFileName,
                    &globalFrame.Screen_Width, &globalFrame.Screen_Height,
                    globalRenderingConfiguration.fileBufferSize, ImageFileHandle::READ_MODE) != 1) {
                Logger::error("Error opening continue trace output file\n");
                fprintf(
                    stderr, "Opening new output file %s.\n", globalRenderingConfiguration.outputFileName);
                globalRenderingConfiguration.options &= ~CONTINUE_TRACE;

                if (globalRenderingConfiguration.outputFileInputStream->open(globalRenderingConfiguration.outputFileName,
                        &globalFrame.Screen_Width, &globalFrame.Screen_Height,
                        globalRenderingConfiguration.fileBufferSize, ImageFileHandle::WRITE_MODE) != 1) {
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
                    globalRenderingConfiguration.fileBufferSize, ImageFileHandle::WRITE_MODE) != 1) {
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
        Logger::info("Rendering...\n");
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
    printStatistics(globalStatistics, globalFrame, globalRenderingConfiguration);

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
            strncpy(globalRenderingConfiguration.statFileName, &optionString[1], RENDER_FILE_NAME_LENGTH - 1);
            globalRenderingConfiguration.statFileName[RENDER_FILE_NAME_LENGTH - 1] = '\0';
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
            strncpy(globalRenderingConfiguration.inputFileName, &optionString[1], RENDER_FILE_NAME_LENGTH - 1);
            globalRenderingConfiguration.inputFileName[RENDER_FILE_NAME_LENGTH - 1] = '\0';
        }
        optionNumber = 0;
        break;

    case 'O':
    case 'o':
        if (optionString[1] == '\0') {
            outFlag = TRUE;
        } else {
            strncpy(globalRenderingConfiguration.outputFileName, &optionString[1], RENDER_FILE_NAME_LENGTH - 1);
            globalRenderingConfiguration.outputFileName[RENDER_FILE_NAME_LENGTH - 1] = '\0';
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
    Logger::info( "\nPOV-Ray          Options in effect: ");

    if (globalRenderingConfiguration.options & CONTINUE_TRACE) {
        Logger::info( "+c ");
    }

    if (globalRenderingConfiguration.options & DISPLAY) {
        Logger::info( "+d%c%c ", globalRenderingConfiguration.displayFormat, globalRenderingConfiguration.paletteOption);
    }

    if (globalRenderingConfiguration.options & VERBOSE) {
        Logger::info( "+v%c ", globalRenderingConfiguration.verboseFormat);
    }

    if (globalRenderingConfiguration.options & VERBOSE_FILE) {
        Logger::info( "+@%s ", globalRenderingConfiguration.statFileName);
    }

    if (globalRenderingConfiguration.options & DISKWRITE) {
        Logger::info( "+f%c ", globalRenderingConfiguration.outputFormat);
    }

    if (globalRenderingConfiguration.options & PROMPTEXIT) {
        Logger::info( "+p ");
    }

    if (globalRenderingConfiguration.options & EXITENABLE) {
        Logger::info( "+x ");
    }

    if (globalRenderingConfiguration.options & ANTIALIAS) {
        Logger::info( "+a%f ", globalRenderingConfiguration.antialiasThreshold);
    }

    if (globalRenderingConfiguration.options & DEBUGGING) {
        Logger::info( "+z ");
    }

    if (globalRenderingConfiguration.fileBufferSize != 0) {
        Logger::info( "-b%d ", globalRenderingConfiguration.fileBufferSize / 1024);
    }

    Logger::info( "-q%d -w%d -h%d -s%d -e%d\n-i%s ", globalRenderingConfiguration.quality,
        globalFrame.Screen_Width, globalFrame.Screen_Height, globalRenderingConfiguration.firstLine,
        globalRenderingConfiguration.lastLine, globalRenderingConfiguration.inputFileName);

    if (globalRenderingConfiguration.options & DISKWRITE) {
        Logger::info( "-o%s ", globalRenderingConfiguration.outputFileName);
    }

    const java::ArrayList<java::String> &paths = FileLocator::searchPaths();
    for (long int i = 0; i < paths.size(); i++) {
        Logger::info("-l%s ", paths.get(i).toCString());
    }

    Logger::info( "\n");
}

void
PovrayApplication::parseFileName(char *fileName)
{
    FILE *defaultsFile;
    char optionString[256];

    if (inFlag) /* file names may now be separated by spaces from cmdline option
                 */
    {
        strncpy(globalRenderingConfiguration.inputFileName, fileName, RENDER_FILE_NAME_LENGTH - 1);
        globalRenderingConfiguration.inputFileName[RENDER_FILE_NAME_LENGTH - 1] = '\0';
        inFlag = FALSE;
        return;
    }

    if (outFlag) /* file names may now be separated by spaces from cmdline
                    option */
    {
        strncpy(globalRenderingConfiguration.outputFileName, fileName, RENDER_FILE_NAME_LENGTH - 1);
        globalRenderingConfiguration.outputFileName[RENDER_FILE_NAME_LENGTH - 1] = '\0';
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

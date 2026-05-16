/****************************************************************************
 *                     povray.c
 *
 *  This module contains the entry routine for the raytracer and the code to
 *  parse the parameters on the command line.
 *
 *****************************************************************************/
#include "app/Unix.h"
#include "app/PovApp.h"
#include "common/Frame.h" /* common to ALL modules in this program */
#include "common/PovProto.h"
#include "geom/PrioQ.h"
#include "io/Parse.h"
#include "render/Render.h"
#include <cctype>
#include <ctime> /* BP */

static constexpr int MAX_FILE_NAMES = 1;
unsigned int Options;
int quality;
int caseSensitiveFlag = CASE_SENSITIVE_DEFAULT;

FILE *bfp;

extern Frame globalFrame;
extern PriorityQueue *GLOBAL_priorityQueue;
PriorityQueueNode *GLOBAL_priorityQueuesHead;

char inputFileName[FILE_NAME_LENGTH];
char outputFileName[FILE_NAME_LENGTH];
char statFileName[FILE_NAME_LENGTH];

static constexpr int MAX_LIBRARIES = 10;
char *libraryPaths[MAX_LIBRARIES];
int libraryPathIndex;

int maxSymbols = 500;

FileHandle *globalOutputFileHandle;
int fileBufferSize;
static int numberOfFiles;
static int inFlag;
static int outFlag;
DBL VTemp;
DBL antialiasThreshold;
int firstLine, lastLine;
int displayStarted = FALSE;
int shadowTestFlag = FALSE;

/* Stats kept by the ray tracer: */
long numberOfPixels;
long numberOfRays;
long numberOfPixelsSupersampled;
long raySphereTests;
long raySphereTestsSucceeded;
long rayBoxTests;
long rayBoxTestsSucceeded;
long rayBlobTests;
long rayBlobTestsSucceeded;
long rayPlaneTests;
long rayPlaneTestsSucceeded;
long rayTriangleTests;
long rayTriangleTestsSucceeded;
long rayQuadricTests;
long rayQuadricTestsSucceeded;
long rayPolyTests;
long rayPolyTestsSucceeded;
long rayBicubicTests;
long rayBicubicTestsSucceeded;
long rayHtFieldTests;
long rayHtFieldTestsSucceeded;
long rayHtFieldBoxTests;
long rayHFieldBoxTestsSucceeded;
long boundingRegionTests;
long boundingRegionTestsSucceeded;
long clippingRegionTests;
long clippingRegionTestsSucceeded;
long callsToNoise;
long callsToDNoise;
long shadowRayTests;
long shadowRaysSucceeded;
long reflectedRaysTraced;
long refractedRaysTraced;
long transmittedRaysTraced;
time_t tstart;
time_t tstop;
DBL tused; /* Trace timer variables. - BP */

char displayFormat;
char outputFormat;
char verboseFormat;
char paletteOption;
char colorBits;

int
main(int argc, char *argv[])
{
    unixInitPovray();
    PovApp::initializeFromCommandLine(argc, argv);
    PovApp::configureOutputTarget();
    PovApp::parseSceneDescription();
    PovApp::prepareRendering();
    PovApp::runRenderLoop();
    PovApp::finalizeRun();

    return 0;
}

void
PovApp::initializeFromCommandLine(int argc, char *argv[])
{
    register int i;

    if (argc == 1) {
        usage();
    }

    initVars();
    outputFileName[0] = '\0';
    libraryPaths[0] = nullptr;
    libraryPathIndex = 0;

    getDefaults();

    for (i = 1; i < argc; i++) {
        if ((*argv[i] == '+') || (*argv[i] == '-')) {
            parseOption(argv[i]);
        } else {
            parseFileName(argv[i]);
        }
    }

    if (lastLine == -1) {
        lastLine = globalFrame.Screen_Height;
    }
}

void
PovApp::configureOutputTarget()
{
    if (!(Options & DISKWRITE)) {
        return;
    }

    switch (outputFormat) {
    case '\0':
    case 'd':
    case 'D':
        if ((globalOutputFileHandle = getDumpFileHandle()) == nullptr) {
            closeAll();
            exit(1);
        }
        break;
    case 'r':
    case 'R':
        if ((globalOutputFileHandle = getRawFileHandle()) == nullptr) {
            closeAll();
            exit(1);
        }
        break;
    case 't':
    case 'T':
        if ((globalOutputFileHandle = getTargaFileHandle()) == nullptr) {
            closeAll();
            exit(1);
        }
        break;
    default:
        fprintf(stderr, "Unrecognized output file format %c\n", outputFormat);
        exit(1);
    }

    if (outputFileName[0] == '\0') {
        strcpy(outputFileName, defaultFileName(globalOutputFileHandle));
    }
}

void
PovApp::parseSceneDescription()
{
    FILE *statFile;

    printOptions();

    initializeTokenizer(inputFileName);
    fprintf(stderr, "Parsing...");
    if (Options & VERBOSE_FILE) {
        statFile = fopen(statFileName, "w+t");
        fprintf(statFile, "Parsing...\n");
        fclose(statFile);
    }

    Parse(&globalFrame);
    terminateTokenizer();
}

void
PovApp::prepareRendering()
{
    if (Options & DISPLAY) {
        printf("Displaying...\n");
        displayInit(globalFrame.Screen_Width, globalFrame.Screen_Height);
        displayStarted = TRUE;
    }

    if (Options & DISKWRITE) {
        if (Options & CONTINUE_TRACE) {
            if (openFile(globalOutputFileHandle, outputFileName,
                    &globalFrame.Screen_Width, &globalFrame.Screen_Height,
                    fileBufferSize, READ_MODE) != 1) {
                fprintf(stderr, "Error opening continue trace output file\n");
                fprintf(stderr, "Opening new output file %s.\n", outputFileName);
                Options &= ~CONTINUE_TRACE;

                if (openFile(globalOutputFileHandle, outputFileName,
                        &globalFrame.Screen_Width, &globalFrame.Screen_Height,
                        fileBufferSize, WRITE_MODE) != 1) {
                    fprintf(stderr, "Error opening output file\n");
                    closeAll();
                    exit(1);
                }
            }

            initializeRenderer();
            if (Options & CONTINUE_TRACE) {
                readRenderedPart();
            }
        } else {
            if (openFile(globalOutputFileHandle, outputFileName,
                    &globalFrame.Screen_Width, &globalFrame.Screen_Height,
                    fileBufferSize, WRITE_MODE) != 1) {
                fprintf(stderr, "Error opening output file\n");
                closeAll();
                exit(1);
            }

            initializeRenderer();
        }
    } else {
        initializeRenderer();
    }

    GLOBAL_priorityQueuesHead = pqInit();
    initializeNoise();
}

void
PovApp::runRenderLoop()
{
    FILE *statFile;

    time(&tstart);

    if ((Options & VERBOSE) && (verboseFormat != '1')) {
        printf("Rendering...\n");
    } else if ((Options & VERBOSE) && (verboseFormat == '1')) {
        fprintf(stderr, "POV-Ray rendering %s to %s :\n", inputFileName,
            outputFileName);
    }
    if (Options & VERBOSE_FILE) {
        statFile = fopen(statFileName, "w+t");
        fprintf(statFile, "Parsed ok. Now rendering %s to %s :\n", inputFileName,
            outputFileName);
        fclose(statFile);
    }

    startTracing();

    if (Options & VERBOSE && verboseFormat == '1') {
        fprintf(stderr, "\n");
    }
}

void
PovApp::finalizeRun()
{
    FILE *statFile;

    time(&tstop);
    tused = (tstop - tstart);

    displayFinished();
    closeAll();
    printStats();

    if (Options & VERBOSE_FILE) {
        statFile = fopen(statFileName, "a+t");
        fprintf(statFile, "Done Tracing\n");
        fclose(statFile);
    }
}

/* Print out usage error message */
void
PovApp::usage()
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
PovApp::initVars()
{
    globalOutputFileHandle = nullptr;
    fileBufferSize = 0;
    Options = 0;
    quality = 9;
    numberOfFiles = 0;
    firstLine = 0;
    lastLine = -1;
    colorBits = 8;

    numberOfPixels = 0L;
    numberOfRays = 0L;
    numberOfPixelsSupersampled = 0L;
    rayHtFieldTests = 0L;
    rayHtFieldTestsSucceeded = 0L;
    rayHtFieldBoxTests = 0L;
    rayHFieldBoxTestsSucceeded = 0L;
    rayBicubicTests = 0L;
    rayBicubicTestsSucceeded = 0L;
    rayBlobTests = 0L;
    rayBlobTestsSucceeded = 0L;
    rayBoxTests = 0L;
    rayBoxTestsSucceeded = 0L;
    raySphereTests = 0L;
    raySphereTestsSucceeded = 0L;
    rayPlaneTests = 0L;
    rayPlaneTestsSucceeded = 0L;
    rayTriangleTests = 0L;
    rayTriangleTestsSucceeded = 0L;
    rayQuadricTests = 0L;
    rayQuadricTestsSucceeded = 0L;
    rayPolyTests = 0L;
    rayPolyTestsSucceeded = 0L;
    boundingRegionTests = 0L;
    boundingRegionTestsSucceeded = 0L;
    clippingRegionTests = 0L;
    clippingRegionTestsSucceeded = 0L;
    callsToNoise = 0L;
    callsToDNoise = 0L;
    shadowRayTests = 0L;
    shadowRaysSucceeded = 0L;
    reflectedRaysTraced = 0L;
    refractedRaysTraced = 0L;
    transmittedRaysTraced = 0L;

    globalFrame.Screen_Height = 100;
    globalFrame.Screen_Width = 100;

    antialiasThreshold = 0.3;
    strcpy(inputFileName, "object.dat");
}

/* Close all the stuff that has been opened. */
void
PovApp::closeAll()
{
    if ((Options & DISPLAY) && displayStarted) {
        displayClose();
    }

    if (globalOutputFileHandle) {
        closeFile(globalOutputFileHandle);
    }
}

/* Read the default parameters from povray.def */
void
PovApp::getDefaults()
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
    Options |= DISKWRITE;
    outputFormat = DEFAULT_OUTPUT_FORMAT;

    if ((Option_String_Ptr = getenv("POVRAYOPT")) != nullptr) {
        PovApp::readOptions(Option_String_Ptr);
    }
    if ((defaultsFile = PovApp::locateFile("povray.def", "r")) != nullptr) {
        while (fgets(optionString, 256, defaultsFile) != nullptr) {
            PovApp::readOptions(optionString);
        }
        fclose(defaultsFile);
    }
}

void
PovApp::readOptions(char *optionLine)
{
    register int c;
    register int stringIndex;
    register int optionStarted;
    short optionLineIndex = 0;
    char optionString[80];

    stringIndex = 0;
    optionStarted = FALSE;
    while ((c = optionLine[optionLineIndex++]) != '\0') {
        if (optionStarted) {
            if (isspace(c)) {
                optionString[stringIndex] = '\0';
                PovApp::parseOption(optionString);
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
                fprintf(stderr,
                    "\nBad default file format.  Offending char: (%c), val: "
                    "%d.\n",
                    (char)c, c);
                exit(1);
            }
    }

    if (optionStarted) {
        optionString[stringIndex] = '\0';
        PovApp::parseOption(optionString);
    }
}

/* parse the command line parameters */
void
PovApp::parseOption(char *optionString)
{
    register int addOption;
    unsigned int optionNumber = 0;
    DBL threshold;

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
        sscanf(&optionString[1], "%d", &fileBufferSize);
        fileBufferSize *= 1024;
        if (fileBufferSize < BUFSIZ) {
            fileBufferSize = BUFSIZ;
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
        displayFormat = '0';
        paletteOption = '3';
        if (optionString[1] != '\0') {
            displayFormat = (char)toupper(optionString[1]);
        }

        if (optionString[1] != '\0' && optionString[2] != '\0') {
            paletteOption = (char)toupper(optionString[2]);
        }
        break;

    case '@':
        optionNumber = VERBOSE_FILE;
        if (optionString[1] == '\0') {
            strcpy(statFileName, "POVSTAT.OUT");
        } else {
            strncpy(statFileName, &optionString[1], FILE_NAME_LENGTH - 1);
            statFileName[FILE_NAME_LENGTH - 1] = '\0';
        }
        break;
    case 'V':
    case 'v':
        optionNumber = VERBOSE;
        verboseFormat = (char)toupper(optionString[1]);
        if (verboseFormat == '\0') {
            verboseFormat = '1';
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
            outputFormat = (char)tolower(optionString[1]);
        } else {
            outputFormat = optionString[1];
        }

        /* Default the output format to the default in the config file */
        if (outputFormat == '\0') {
            outputFormat = DEFAULT_OUTPUT_FORMAT;
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
            strncpy(inputFileName, &optionString[1], FILE_NAME_LENGTH - 1);
            inputFileName[FILE_NAME_LENGTH - 1] = '\0';
        }
        optionNumber = 0;
        break;

    case 'O':
    case 'o':
        if (optionString[1] == '\0') {
            outFlag = TRUE;
        } else {
            strncpy(outputFileName, &optionString[1], FILE_NAME_LENGTH - 1);
            outputFileName[FILE_NAME_LENGTH - 1] = '\0';
        }
        optionNumber = 0;
        break;

    case 'A':
    case 'a':
        optionNumber = ANTIALIAS;
        if (sscanf(&optionString[1], DBL_FORMAT_STRING, &threshold) != EOF) {
            antialiasThreshold = threshold;
        }
        break;

    case 'X':
    case 'x':
        optionNumber = EXITENABLE;
        break;

    case 'L':
    case 'l':
        if (libraryPathIndex >= MAX_LIBRARIES) {
            fprintf(stderr, "Too many library directories specified\n");
            exit(1);
        }
        libraryPaths[libraryPathIndex] = new char[strlen(optionString) + 1];
        if (libraryPaths[libraryPathIndex] == nullptr) {
            fprintf(stderr,
                "Out of memory. Cannot allocate memory for library pathname\n");
            exit(1);
        }
        strcpy(libraryPaths[libraryPathIndex], &optionString[1]);
        libraryPathIndex++;
        optionNumber = 0;
        break;
    case 'T':
    case 't':
        switch (toupper(optionString[1])) {
        case 'Y':
            caseSensitiveFlag = 0;
            break;
        case 'N':
            caseSensitiveFlag = 1;
            break;
        case 'O':
            caseSensitiveFlag = 2;
            break;
        default:
            caseSensitiveFlag = 2;
            break;
        }
        optionNumber = 0;
        break;

    case 'S':
    case 's':
        sscanf(&optionString[1], "%d", &firstLine);
        optionNumber = 0;
        break;

    case 'E':
    case 'e':
        sscanf(&optionString[1], "%d", &lastLine);
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
        sscanf(&optionString[1], "%d", &quality);
        optionNumber = 0;
        break;

        /* Turn on debugging print statements. */
    case 'Z':
    case 'z':
        optionNumber = DEBUGGING;
        break;

    default:
        fprintf(stderr, "\nInvalid option: %s\n\n", --optionString);
        optionNumber = 0;
    }

    if (optionNumber != 0) {
        if (addOption) {
            Options |= optionNumber;
        } else {
            Options &= ~optionNumber;
        }
    }
}

void
PovApp::printOptions()
{
    int i;

    fprintf(stdout, "\nPOV-Ray          Options in effect: ");

    if (Options & CONTINUE_TRACE) {
        fprintf(stdout, "+c ");
    }

    if (Options & DISPLAY) {
        fprintf(stdout, "+d%c%c ", displayFormat, paletteOption);
    }

    if (Options & VERBOSE) {
        fprintf(stdout, "+v%c ", verboseFormat);
    }

    if (Options & VERBOSE_FILE) {
        fprintf(stdout, "+@%s ", statFileName);
    }

    if (Options & DISKWRITE) {
        fprintf(stdout, "+f%c ", outputFormat);
    }

    if (Options & PROMPTEXIT) {
        fprintf(stdout, "+p ");
    }

    if (Options & EXITENABLE) {
        fprintf(stdout, "+x ");
    }

    if (Options & ANTIALIAS) {
        fprintf(stdout, "+a%f ", antialiasThreshold);
    }

    if (Options & DEBUGGING) {
        fprintf(stdout, "+z ");
    }

    if (fileBufferSize != 0) {
        fprintf(stdout, "-b%d ", fileBufferSize / 1024);
    }

    fprintf(stdout, "-q%d -w%d -h%d -s%d -e%d\n-i%s ", quality,
        globalFrame.Screen_Width, globalFrame.Screen_Height, firstLine,
        lastLine, inputFileName);

    if (Options & DISKWRITE) {
        fprintf(stdout, "-o%s ", outputFileName);
    }

    for (i = 0; i < libraryPathIndex; i++) {
        fprintf(stdout, "-l%s ", libraryPaths[i]);
    }

    fprintf(stdout, "\n");
}

void
PovApp::parseFileName(char *fileName)
{
    FILE *defaultsFile;
    char optionString[256];

    if (inFlag) /* file names may now be separated by spaces from cmdline option
                 */
    {
        strncpy(inputFileName, fileName, FILE_NAME_LENGTH - 1);
        inputFileName[FILE_NAME_LENGTH - 1] = '\0';
        inFlag = FALSE;
        return;
    }

    if (outFlag) /* file names may now be separated by spaces from cmdline
                    option */
    {
        strncpy(outputFileName, fileName, FILE_NAME_LENGTH - 1);
        outputFileName[FILE_NAME_LENGTH - 1] = '\0';
        outFlag = FALSE;
        return;
    }

    if (++numberOfFiles > MAX_FILE_NAMES) {
        fprintf(stderr,
            "\nOnly %d option file names are allowed in a command line.",
            MAX_FILE_NAMES);
        exit(1);
    }

    if ((defaultsFile = PovApp::locateFile(fileName, "r")) != nullptr) {
        while (fgets(optionString, 256, defaultsFile) != nullptr) {
            PovApp::readOptions(optionString);
        }
        fclose(defaultsFile);
    } else {
        printf("\nError opening option file %s.", fileName);
    }
}

void
PovApp::printStats()
{
    int hours;
    int min;
    DBL sec;
    FILE *statOut;
    long pixelsInImage;

    if (Options & VERBOSE_FILE) {
        statOut = fopen(statFileName, "w+t");
    } else {
        statOut = stdout;
    }

    pixelsInImage =
        (long)globalFrame.Screen_Width * (long)globalFrame.Screen_Height;

    fprintf(statOut, "\n%s statistics\n", inputFileName);
    if (pixelsInImage > numberOfPixels) {
        fprintf(statOut, "  Partial Image Rendered");
    }

    fprintf(statOut, "--------------------------------------\n");
    fprintf(statOut, "Resolution %d x %d\n", globalFrame.Screen_Width,
        globalFrame.Screen_Height);
    fprintf(statOut,
        "# Rays:  %10ld     # Pixels:  %10ld  # Pixels supersampled: %10ld\n",
        numberOfRays, numberOfPixels, numberOfPixelsSupersampled);

    fprintf(statOut, "  Ray->Shape Intersection Tests:\n");
    fprintf(statOut,
        "    Type                 Tests     Succeeded    Percentage\n");
    fprintf(statOut,
        "  -----------------------------------------------------------\n");
    if (raySphereTests) {
        fprintf(statOut, "  Sphere         %10ld  %10ld  %10.2f\n",
            raySphereTests, raySphereTestsSucceeded,
            (((DBL)raySphereTestsSucceeded / (DBL)raySphereTests) * 100.0));
    }
    if (rayPlaneTests) {
        fprintf(statOut, "  Plane          %10ld  %10ld  %10.2f\n",
            rayPlaneTests, rayPlaneTestsSucceeded,
            (((DBL)rayPlaneTestsSucceeded / (DBL)rayPlaneTests) * 100.0));
    }
    if (rayTriangleTests) {
        fprintf(statOut, "  Triangle      %10ld  %10ld  %10.2f\n",
            rayTriangleTests, rayTriangleTestsSucceeded,
            (((DBL)rayTriangleTestsSucceeded / (DBL)rayTriangleTests) * 100.0));
    }
    if (rayQuadricTests) {
        fprintf(statOut, "  Quadric        %10ld  %10ld  %10.2f\n",
            rayQuadricTests, rayQuadricTestsSucceeded,
            (((DBL)rayQuadricTestsSucceeded / (DBL)rayQuadricTests) * 100.0));
    }
    if (rayBlobTests) {
        fprintf(statOut, "  Blob            %10ld  %10ld  %10.2f\n",
            rayBlobTests, rayBlobTestsSucceeded,
            (((DBL)rayBlobTestsSucceeded / (DBL)rayBlobTests) * 100.0));
    }
    if (rayBoxTests) {
        fprintf(statOut, "  Box             %10ld  %10ld  %10.2f\n",
            rayBoxTests, rayBoxTestsSucceeded,
            (((DBL)rayBoxTestsSucceeded / (DBL)rayBoxTests) * 100.0));
    }
    if (rayPolyTests) {
        fprintf(statOut, "  Quartic\\Poly %10ld  %10ld  %10.2f\n", rayPolyTests,
            rayPolyTestsSucceeded,
            (((DBL)rayPolyTestsSucceeded / (DBL)rayPolyTests) * 100.0));
    }
    if (rayBicubicTests) {
        fprintf(statOut, "  Bezier Patch %10ld  %10ld  %10.2f\n",
            rayBicubicTests, rayBicubicTestsSucceeded,
            (((DBL)rayBicubicTestsSucceeded / (DBL)rayBicubicTests) * 100.0));
    }
    if (rayHtFieldTests) {
        fprintf(statOut, "  Height Fld    %10ld  %10ld  %10.2f\n",
            rayHtFieldTests, rayHtFieldTestsSucceeded,
            (((DBL)rayHtFieldTestsSucceeded / (DBL)rayHtFieldTests) * 100.0));
    }
    if (rayHtFieldBoxTests) {
        fprintf(statOut, "  Hght Fld Box %10ld  %10ld  %10.2f\n",
            rayHtFieldBoxTests, rayHFieldBoxTestsSucceeded,
            (((DBL)rayHFieldBoxTestsSucceeded / (DBL)rayHtFieldBoxTests) *
                100.0));
    }
    if (boundingRegionTests) {
        fprintf(statOut, "  Bounds         %10ld  %10ld  %10.2f\n",
            boundingRegionTests, boundingRegionTestsSucceeded,
            (((DBL)boundingRegionTestsSucceeded / (DBL)boundingRegionTests) *
                100.0));
    }
    if (clippingRegionTests) {
        fprintf(statOut, "  Clips          %10ld  %10ld  %10.2f\n",
            clippingRegionTests, clippingRegionTestsSucceeded,
            (((DBL)clippingRegionTestsSucceeded / (DBL)clippingRegionTests) *
                100.0));
    }

    if (callsToNoise) {

        fprintf(statOut, "  Calls to Noise:    %10ld\n", callsToNoise);
    }
    if (callsToDNoise) {
        fprintf(statOut, "  Calls to DNoise:  %10ld\n", callsToDNoise);
    }
    if (shadowRayTests) {
        fprintf(statOut,
            "  Shadow Ray Tests: %10ld      Blocking Objects Found:  %10ld\n",
            shadowRayTests, shadowRaysSucceeded);
    }
    if (reflectedRaysTraced) {
        fprintf(statOut, "  Reflected Rays:    %10ld\n", reflectedRaysTraced);
    }
    if (refractedRaysTraced) {
        fprintf(statOut, "  Refracted Rays:    %10ld\n", refractedRaysTraced);
    }
    if (transmittedRaysTraced) {
        fprintf(statOut, "  Transmitted Rays: %10ld\n", transmittedRaysTraced);
    }

    if (tused == 0) {
        stopTime(&tstop); /* Get trace done time. */
        tused = difftime(tstop, tstart);
        /* Calc. elapsed time. */
        /* 0 in your specific CONFIG.H if unsupported */
    }
    if (tused != 0) {
        /* Convert seconds to hours, min & sec. CdW */
        hours = (int)tused / 3600;
        min = (int)(tused - hours * 3600) / 60;
        sec = tused - (DBL)(hours * 3600 + min * 60);
        fprintf(statOut,
            "  Time For Trace:    %2d hours %2d minutes %4.2f seconds\n", hours,
            min, sec);
    }
    if (Options & VERBOSE_FILE) {
        fclose(statOut);
    }
}

/* Find a file in the search path. */
FILE *
PovApp::locateFile(const char *filename, const char *mode)
{
    FILE *f;
    int i;
    char pathname[FILE_NAME_LENGTH];

    /* Check the current directory first. */
    if ((f = fopen(filename, mode)) != nullptr) {
        return (f);
    }

    for (i = 0; i < libraryPathIndex; i++) {
        strcpy(pathname, libraryPaths[i]);
        if (FILENAME_SEPARATOR != nullptr) {
            strcat(pathname, FILENAME_SEPARATOR);
        }
        strcat(pathname, filename);
        if ((f = fopen(pathname, mode)) != nullptr) {
            return (f);
        }
    }

    return nullptr;
}

void
PovApp::printCredits()
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

/****************************************************************************
 *                         render.c
 *
 *  This module implements the main raytracing loop.
 *
 * 01/16/92 dfm     Added David Buck's bug fix to add a different offset
 *                      to x and y coordinates for each call to Rand3D() in the
 *                      subsampling routine.  Said to smooth the anti-aliasing.
 *                      Previously, each call returned the same random number,
 *                      hence, no true jittering.
 *                      I consider this an interim fix until we get a better
 *                      algorithm for anti-aliasing.
 *****************************************************************************/

#include "render/Render.h"
#include "app/Unix.h"
#include "common/Color.h"
#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "common/VectorOps.h"
#include "io/Dump.h"
#include "render/Lighting.h"

extern FileHandle *globalOutputFileHandle;
extern char outputFileName[FILE_NAME_LENGTH];
extern char inputFileName[FILE_NAME_LENGTH];
extern char statFileName[FILE_NAME_LENGTH];
extern char outputFormat, colorBits, paletteOption;
extern char verboseFormat;
extern unsigned int Options;
extern int fileBufferSize;
extern int quality;
volatile int stopFlag;
extern int firstLine, lastLine;
extern long numberOfPixels, numberOfRays, numberOfPixelsSupersampled;

extern short *hashTable;
extern unsigned short crctab[256];

inline unsigned short
RenderEngine::RenderEngine::rand3dInline(int a, int b)
{
    return crctab[(int)(hashTable[(int)(hashTable[(int)(a & 0xfff)] ^ b) &
                                  0xfff]) &
                  0xff];
}

RenderFrame globalFrame;
Ray *vpRay;
int traceLevel, superSampleCount;

double maxTraceLevel = 5;
double maxclr;

RGBAColor *previousLine, *currentLine;
char *previousLineAntialiasedFlags, *currentLineAntialiasedFlags;
Ray ray;

void
RenderFrame::createRay(Ray *ray, int width, int height, double x, double y)
{
    register double xScalar;
    register double yScalar;
    Vector3D tempVect1;
    Vector3D tempVect2;

    /* Convert the X Coordinate to be a double from 0.0 to 1.0 */
    xScalar = (x - (double)width / 2.0) / (double)width;

    /* Convert the Y Coordinate to be a double from 0.0 to 1.0 */
    yScalar = (((double)(globalFrame.Screen_Height - 1) - y) - (double)height / 2.0) /
              (double)height;

    VectorOps::vScale(tempVect1, globalFrame.View_Point.Up, yScalar);
    VectorOps::vScale(tempVect2, globalFrame.View_Point.Right, xScalar);
    VectorOps::vAdd(ray->Direction, tempVect1, tempVect2);
    VectorOps::vAdd(ray->Direction, ray->Direction, globalFrame.View_Point.Direction);
    VectorOps::vNormalize(ray->Direction, ray->Direction);
    ray->initializeContainers();
    ray->Quadric_Constants_Cached = FALSE;
}

void
RenderEngine::supersample(RGBAColor *result, int x, int y, int width, int height)
{
    RGBAColor colour;
    register double dx;
    register double dy;
    register double jitterX;
    register double jitterY;
    register int jittOffset;
    unsigned char red;
    unsigned char green;
    unsigned char blue;

    dx = (double)x;
    dy = (double)y;
    jittOffset = 10;

    numberOfPixelsSupersampled++;

    Color::makeColor(result, 0.0, 0.0, 0.0);

    jitterX = (RenderEngine::rand3dInline(x + jittOffset, y) & 0x7FFF) / 32768.0 * 0.33333333 -
              0.16666666;
    jitterY = (RenderEngine::rand3dInline(x + jittOffset, y) & 0x7FFF) / 32768.0 * 0.33333333 -
              0.16666666;
    RenderFrame::createRay(vpRay, globalFrame.Screen_Width, globalFrame.Screen_Height,
        dx + jitterX, dy + jitterY);

    traceLevel = 0;
    RenderEngine::trace(vpRay, &colour);
    Color::clipColor(&colour, &colour);
    Color::scaleColor(&colour, &colour, 0.11111111);
    Color::addColor(result, result, &colour);
    jittOffset += 10;

    jitterX = (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) / 32768.0 *
                  0.33333333 -
              0.16666666;
    jitterY = (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) / 32768.0 *
                  0.33333333 -
              0.16666666;
    RenderFrame::createRay(vpRay, width, height, dx + jitterX - 0.3333333,
        dy + jitterY - 0.3333333);
    traceLevel = 0;
    RenderEngine::trace(vpRay, &colour);
    Color::clipColor(&colour, &colour);
    Color::scaleColor(&colour, &colour, 0.11111111);
    Color::addColor(result, result, &colour);
    jittOffset += 10;

    jitterX = (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) / 32768.0 *
                  0.33333333 -
              0.16666666;
    jitterY = (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) / 32768.0 *
                  0.33333333 -
              0.16666666;
    RenderFrame::createRay(vpRay, width, height, dx + jitterX - 0.3333333, dy + jitterY);
    traceLevel = 0;
    RenderEngine::trace(vpRay, &colour);
    Color::clipColor(&colour, &colour);
    Color::scaleColor(&colour, &colour, 0.11111111);
    Color::addColor(result, result, &colour);
    jittOffset += 10;

    jitterX = (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) / 32768.0 *
                  0.33333333 -
              0.16666666;
    jitterY = (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) / 32768.0 *
                  0.33333333 -
              0.16666666;
    RenderFrame::createRay(vpRay, width, height, dx + jitterX - 0.3333333,
        dy + jitterY + 0.3333333);
    traceLevel = 0;
    RenderEngine::trace(vpRay, &colour);
    Color::clipColor(&colour, &colour);
    Color::scaleColor(&colour, &colour, 0.11111111);
    Color::addColor(result, result, &colour);
    jittOffset += 10;

    jitterX = (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) / 32768.0 *
                  0.33333333 -
              0.16666666;
    jitterY = (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) / 32768.0 *
                  0.33333333 -
              0.16666666;
    RenderFrame::createRay(vpRay, width, height, dx + jitterX, dy + jitterY - 0.3333333);
    traceLevel = 0;
    RenderEngine::trace(vpRay, &colour);
    Color::clipColor(&colour, &colour);
    Color::scaleColor(&colour, &colour, 0.11111111);
    Color::addColor(result, result, &colour);
    jittOffset += 10;

    jitterX = (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) / 32768.0 *
                  0.33333333 -
              0.16666666;
    jitterY = (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) / 32768.0 *
                  0.33333333 -
              0.16666666;
    RenderFrame::createRay(vpRay, width, height, dx + jitterX, dy + jitterY + 0.3333333);
    traceLevel = 0;
    RenderEngine::trace(vpRay, &colour);
    Color::clipColor(&colour, &colour);
    Color::scaleColor(&colour, &colour, 0.11111111);
    Color::addColor(result, result, &colour);
    jittOffset += 10;

    jitterX = (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) / 32768.0 *
                  0.33333333 -
              0.16666666;
    jitterY = (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) / 32768.0 *
                  0.33333333 -
              0.16666666;
    RenderFrame::createRay(vpRay, width, height, dx + jitterX + 0.3333333,
        dy + jitterY - 0.3333333);
    traceLevel = 0;
    RenderEngine::trace(vpRay, &colour);
    Color::clipColor(&colour, &colour);
    Color::scaleColor(&colour, &colour, 0.11111111);
    Color::addColor(result, result, &colour);
    jittOffset += 10;

    jitterX = (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) / 32768.0 *
                  0.33333333 -
              0.16666666;
    jitterY = (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) / 32768.0 *
                  0.33333333 -
              0.16666666;
    RenderFrame::createRay(vpRay, width, height, dx + jitterX + 0.3333333, dy + jitterY);
    traceLevel = 0;
    RenderEngine::trace(vpRay, &colour);
    Color::clipColor(&colour, &colour);
    Color::scaleColor(&colour, &colour, 0.11111111);
    Color::addColor(result, result, &colour);
    jittOffset += 10;

    jitterX = (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) / 32768.0 *
                  0.33333333 -
              0.16666666;
    jitterY = (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) / 32768.0 *
                  0.33333333 -
              0.16666666;
    RenderFrame::createRay(vpRay, width, height, dx + jitterX + 0.3333333,
        dy + jitterY + 0.3333333);
    traceLevel = 0;
    RenderEngine::trace(vpRay, &colour);
    Color::clipColor(&colour, &colour);
    Color::scaleColor(&colour, &colour, 0.11111111);
    Color::addColor(result, result, &colour);

    if ((y != firstLine - 1) && (Options & DISPLAY)) {
        red = (unsigned char)(result->Red * maxclr);
        green = (unsigned char)(result->Green * maxclr);
        blue = (unsigned char)(result->Blue * maxclr);
        UnixPlatform::displayPlot(x, y, red, green, blue);
    }
}

void
RenderEngine::readRenderedPart()
{
    int rc;
    int x;
    int lineNumber;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    double grey;

    maxclr = (double)(1 << colorBits) - 1.0;
    while ((rc = DumpFormat::readLine(
                globalOutputFileHandle, previousLine, &lineNumber)) == 1) {
        if (Options & DISPLAY) {
            for (x = 0; x < globalFrame.Screen_Width; x++) {
                if (paletteOption == GREY) {
                    grey = previousLine[x].Red * 0.287 +
                           previousLine[x].Green * 0.589 +
                           previousLine[x].Blue * 0.114;
                    red = green = blue = (unsigned char)(grey * maxclr);
                } else {
                    red = (unsigned char)(previousLine[x].Red * maxclr);
                    green = (unsigned char)(previousLine[x].Green * maxclr);
                    blue = (unsigned char)(previousLine[x].Blue * maxclr);
                }
                UnixPlatform::displayPlot(x, lineNumber, red, green, blue);
                cooperate(); /* Moved inside loop JLN 12/91 */
            }
        }
    }

    firstLine = lineNumber + 1;

    if (rc == 0) {
        DumpFormat::closeFile(globalOutputFileHandle);
        if (openFile(globalOutputFileHandle, outputFileName,
                &globalFrame.Screen_Width, &globalFrame.Screen_Height,
                fileBufferSize, APPEND_MODE) != 1) {
            fprintf(stderr, "Error opening output file\n");
            exit(1);
        }
        return;
    }

    fprintf(stderr, "Error reading aborted data file\n");
}

void
RenderEngine::startTracing()
{
    RGBAColor colour;
    register int x;
    register int y;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    double grey;

    for (y = (Options & ANTIALIAS) ? firstLine - 1 : firstLine; y < lastLine;
         y++) {

        RenderFrame::checkStats(y);

        for (x = 0; x < globalFrame.Screen_Width; x++) {

            testAbort();

            if (stopFlag) {
                PovApp::closeAll();
                printStatsInline();
                /* exit with error if image not completed/user abort*/
                exit(2);
            }

            numberOfPixels++;

            RenderFrame::createRay(vpRay, globalFrame.Screen_Width,
                globalFrame.Screen_Height, (double)x, (double)y);
            traceLevel = 0;
            RenderEngine::trace(&ray, &colour);
            Color::clipColor(&colour, &colour);

            currentLine[x] = colour;

            if (Options & ANTIALIAS) {
                RenderFrame::doAntiAliasing(x, y, &colour);
            }

            if (y != firstLine - 1) {
                if (paletteOption == GREY) {
                    grey = previousLine[x].Red * 0.287 +
                           previousLine[x].Green * 0.589 +
                           previousLine[x].Blue * 0.114;
                    red = green = blue = (unsigned char)(grey * maxclr);
                } else {
                    red = (unsigned char)(colour.Red * maxclr);
                    green = (unsigned char)(colour.Green * maxclr);
                    blue = (unsigned char)(colour.Blue * maxclr);
                }
                if (Options & DISPLAY) {
                    UnixPlatform::displayPlot(x, y, red, green, blue);
                }
            }
        }
        RenderFrame::outputLine(y);
    }

    if (Options & DISKWRITE) {
        DumpFormat::writeLine(globalOutputFileHandle, previousLine, lastLine - 1);
    }
}

void
RenderFrame::checkStats(register int y)
{
    FILE *statFile;

    /* New verbose options CdW */
    if (Options & VERBOSE && verboseFormat == '0') {
        printf("POV-Ray rendering %s to %s", inputFileName, outputFileName);
        if ((firstLine != 0) || (lastLine != globalFrame.Screen_Height)) {
            printf(" from %4d to %4d:\n", firstLine, lastLine);
        } else {
            printf(":\n");
        }
        printf("Res %4d X %4d. Calc line %4d of %4d", globalFrame.Screen_Width,
            globalFrame.Screen_Height, (y - firstLine) + 1,
            lastLine - firstLine);
        if (!(Options & ANTIALIAS)) {
            printf(".");
        }
    }
    if (Options & VERBOSE_FILE) {
        statFile = fopen(statFileName, "w+t");
        fprintf(statFile, "Line %4d.\n", y);
        fclose(statFile);
    }

    /* Use -vO for Old style verbose */
    if (Options & VERBOSE && (verboseFormat == 'O')) {
        printf("Line %4d", y);
    }
    if (Options & VERBOSE && verboseFormat == '1') {
        fprintf(stderr, "Res %4d X %4d. Calc line %4d of %4d",
            globalFrame.Screen_Width, globalFrame.Screen_Height,
            (y - firstLine) + 1, lastLine - firstLine);
        if (!(Options & ANTIALIAS)) {
            fprintf(stderr, ".");
        }
    }

    if (Options & ANTIALIAS) {
        superSampleCount = 0;
    }
}

void
RenderFrame::doAntiAliasing(register int x, register int y, RGBAColor *colour)
{
    char antialiasCenterFlag = 0;

    currentLineAntialiasedFlags[x] = 0;

    if (x != 0) {
        if (Color::colorDistance(&currentLine[x - 1], &currentLine[x]) >=
            globalFrame.Antialias_Threshold) {
            antialiasCenterFlag = 1;
            if (!(currentLineAntialiasedFlags[x - 1])) {
                RenderEngine::supersample(&currentLine[x - 1], x - 1, y,
                    globalFrame.Screen_Width, globalFrame.Screen_Height);
                currentLineAntialiasedFlags[x - 1] = 1;
                superSampleCount++;
            }
        }
    }

    if (y != firstLine - 1) {
        if (Color::colorDistance(&previousLine[x], &currentLine[x]) >=
            globalFrame.Antialias_Threshold) {
            antialiasCenterFlag = 1;
            if (!(previousLineAntialiasedFlags[x])) {
                RenderEngine::supersample(&previousLine[x], x, y - 1,
                    globalFrame.Screen_Width, globalFrame.Screen_Height);
                previousLineAntialiasedFlags[x] = 1;
                superSampleCount++;
            }
        }
    }

    if (antialiasCenterFlag) {
        RenderEngine::supersample(&currentLine[x], x, y, globalFrame.Screen_Width,
            globalFrame.Screen_Height);
        currentLineAntialiasedFlags[x] = 1;
        *colour = currentLine[x];
        superSampleCount++;
    }
}

void
RenderEngine::initializeRenderer()
{
    register int i;

    vpRay = &ray;
    maxclr = (double)(1 << colorBits) - 1.0;

    previousLine = new RGBAColor[(globalFrame.Screen_Width + 1)];
    currentLine = new RGBAColor[(globalFrame.Screen_Width + 1)];

    for (i = 0; i <= globalFrame.Screen_Width; i++) {
        previousLine[i].Red = 0.0;
        previousLine[i].Green = 0.0;
        previousLine[i].Blue = 0.0;

        currentLine[i].Red = 0.0;
        currentLine[i].Green = 0.0;
        currentLine[i].Blue = 0.0;
    }

    if (Options & ANTIALIAS) {
        previousLineAntialiasedFlags = new char[(globalFrame.Screen_Width + 1)];
        currentLineAntialiasedFlags = new char[(globalFrame.Screen_Width + 1)];

        for (i = 0; i <= globalFrame.Screen_Width; i++) {
            (previousLineAntialiasedFlags)[i] = 0;
            (currentLineAntialiasedFlags)[i] = 0;
        }
    }

    ray.Initial = globalFrame.View_Point.Location;
}

void
RenderFrame::outputLine(register int y)
{
    RGBAColor *tempColourPtr;
    char *tempCharPtr;

    if (Options & DISKWRITE) {
        if (y > firstLine) {
            DumpFormat::writeLine(globalOutputFileHandle, previousLine, y - 1);
        }
    }

    if (Options & VERBOSE) {
        if (Options & ANTIALIAS && verboseFormat != '1') {
            printf(" supersampled %d times.", superSampleCount);
        }

        if (Options & ANTIALIAS && verboseFormat == '1') {
            fprintf(stderr, " supersampled %d times.", superSampleCount);
        }
        if (verboseFormat == '1') {
            fprintf(stderr, "\r");
        } else {
            fprintf(stderr, "\n");
        }
    }
    tempColourPtr = previousLine;
    previousLine = currentLine;
    currentLine = tempColourPtr;

    tempCharPtr = previousLineAntialiasedFlags;
    previousLineAntialiasedFlags = currentLineAntialiasedFlags;
    currentLineAntialiasedFlags = tempCharPtr;
}

void
RenderEngine::trace(Ray *ray, RGBAColor *colour)
{
    SimpleBody *object;
    Intersection *localIntersection;
    Intersection *newIntersection;
    register int intersectionFound;

    cooperate();
    numberOfRays++;
    Color::makeColor(colour, 0.0, 0.0, 0.0);

    intersectionFound = FALSE;
    localIntersection = nullptr;

    if (traceLevel > (int)maxTraceLevel) {
        return;
    }

    if (globalFrame.Fog_Distance == 0.0) {
        Color::makeColor(colour, 0.0, 0.0, 0.0);
    } else {
        *colour = globalFrame.Fog_Colour;
    }

    if (Options & DEBUGGING) {
        printf("Calculating intersections level %d\n", traceLevel);
    }

    /* What objects does this ray intersect? */
    for (object = globalFrame.Objects; object != nullptr;
         object = object->Next_Object) {
        cooperate();
        if ((newIntersection = GeometryOps::intersect(object, ray)) != nullptr) {
            if (intersectionFound) {
                if (localIntersection->Depth > newIntersection->Depth) {
                    delete localIntersection;
                    localIntersection = newIntersection;
                } else {
                    delete newIntersection;
                }
            } else {
                localIntersection = newIntersection;
            }

            intersectionFound = TRUE;
        }
    }

    if (intersectionFound) {
        determineSurfaceColour(localIntersection, colour, ray, FALSE);
        delete localIntersection;
    }
}

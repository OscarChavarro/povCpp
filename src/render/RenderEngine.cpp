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

#include "render/RenderEngine.h"
#include "common/logger/Logger.h"
#include "common/color/Color.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "render/RenderOutput.h"
#include "java/io/FileOutputStream.h"
#include "render/RayShaderPipeline.h"
#include "render/shaders/TraceService.h"
#include "environment/material/RendererConfiguration.h"
#include "common/Statistics.h"

volatile int RenderEngine::sStopFlag = 0;
RenderFrame RenderEngine::sRenderFrame;
RayWithSegments *RenderEngine::sPrimaryRay = nullptr;
int RenderEngine::sTraceLevel = 0;
double RenderEngine::sMaxTraceLevel = 5.0;

extern short *hashTable;
extern unsigned short crctab[256];

inline unsigned short
RenderEngine::RenderEngine::rand3dInline(int a, int b)
{
    return crctab[(int)(hashTable[(int)(hashTable[(int)(a & 0xfff)] ^ b) &
                                  0xfff]) &
                  0xff];
}

int superSampleCount;

RGBAColor *previousLine;
RGBAColor *currentLine;
char *previousLineAntialiasedFlags;
char *currentLineAntialiasedFlags;
RayWithSegments ray;

RenderFrame &
RenderEngine::renderFrame()
{
    return sRenderFrame;
}

RayWithSegments *&
RenderEngine::primaryRay()
{
    return sPrimaryRay;
}

int &
RenderEngine::traceLevel()
{
    return sTraceLevel;
}

double &
RenderEngine::maxTraceLevel()
{
    return sMaxTraceLevel;
}

volatile int &
RenderEngine::stopFlag()
{
    return sStopFlag;
}

static const TraceService *getTraceService();

static void
traceServiceTrace(void *context, RayWithSegments *ray, RGBAColor *colour)
{
    (void)context;
    RenderEngine::trace(ray, colour);
}

static void
traceServiceShadeShadow(
    void *context, Intersection *intersection, RGBAColor *colour)
{
    (void)context;
    RayShaderPipeline::shadeSurface(
        intersection, colour, nullptr, TRUE, getTraceService());
}

static const TraceService traceService = {
    traceServiceTrace,
    traceServiceShadeShadow,
    nullptr
};

static const TraceService *
getTraceService()
{
    return &traceService;
}

void
RenderFrame::createRay(
    RayWithSegments *ray, int width, int height, double x, double y)
{
    double xScalar;
    double yScalar;
    Vector3Dd tempVect1;
    Vector3Dd tempVect2;

    /* Convert the X Coordinate to be a double from 0.0 to 1.0 */
    xScalar = (x - (double)width / 2.0) / (double)width;

    /* Convert the Y Coordinate to be a double from 0.0 to 1.0 */
    yScalar =
        (((double)(RenderEngine::renderFrame().screenHeight - 1) - y) - (double)height / 2.0) /
        (double)height;

    VectorOps::vScale(tempVect1, RenderEngine::renderFrame().viewPoint.Up, yScalar);
    VectorOps::vScale(tempVect2, RenderEngine::renderFrame().viewPoint.Right, xScalar);
    VectorOps::vAdd(ray->direction, tempVect1, tempVect2);
    ray->direction.add(RenderEngine::renderFrame().viewPoint.Direction);
    ray->direction.normalize();
    ray->initializeContainers();
    ray->quadricConstantsCached = FALSE;
}

void
RenderEngine::supersample(
    RGBAColor *result, int x, int y, int width, int height)
{
    RGBAColor colour;
    double dx;
    double dy;
    double jitterX;
    double jitterY;
    int jittOffset;

    dx = (double)x;
    dy = (double)y;
    jittOffset = 10;

    globalStatistics.numberOfPixelsSupersampled++;

    Color::makeColor(result, 0.0, 0.0, 0.0);

    jitterX = (RenderEngine::rand3dInline(x + jittOffset, y) & 0x7FFF) /
                  32768.0 * 0.33333333 -
              0.16666666;
    jitterY = (RenderEngine::rand3dInline(x + jittOffset, y) & 0x7FFF) /
                  32768.0 * 0.33333333 -
              0.16666666;
    RenderFrame::createRay(RenderEngine::primaryRay(), RenderEngine::renderFrame().screenWidth,
        RenderEngine::renderFrame().screenHeight, dx + jitterX, dy + jitterY);

    RenderEngine::traceLevel() = 0;
    RenderEngine::trace(RenderEngine::primaryRay(), &colour);
    Color::clipColor(&colour, &colour);
    Color::scaleColor(&colour, &colour, 0.11111111);
    Color::addColor(result, result, &colour);
    jittOffset += 10;

    jitterX =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    RenderFrame::createRay(RenderEngine::primaryRay(), width, height, dx + jitterX - 0.3333333,
        dy + jitterY - 0.3333333);
    RenderEngine::traceLevel() = 0;
    RenderEngine::trace(RenderEngine::primaryRay(), &colour);
    Color::clipColor(&colour, &colour);
    Color::scaleColor(&colour, &colour, 0.11111111);
    Color::addColor(result, result, &colour);
    jittOffset += 10;

    jitterX =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    RenderFrame::createRay(
        RenderEngine::primaryRay(), width, height, dx + jitterX - 0.3333333, dy + jitterY);
    RenderEngine::traceLevel() = 0;
    RenderEngine::trace(RenderEngine::primaryRay(), &colour);
    Color::clipColor(&colour, &colour);
    Color::scaleColor(&colour, &colour, 0.11111111);
    Color::addColor(result, result, &colour);
    jittOffset += 10;

    jitterX =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    RenderFrame::createRay(RenderEngine::primaryRay(), width, height, dx + jitterX - 0.3333333,
        dy + jitterY + 0.3333333);
    RenderEngine::traceLevel() = 0;
    RenderEngine::trace(RenderEngine::primaryRay(), &colour);
    Color::clipColor(&colour, &colour);
    Color::scaleColor(&colour, &colour, 0.11111111);
    Color::addColor(result, result, &colour);
    jittOffset += 10;

    jitterX =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    RenderFrame::createRay(
        RenderEngine::primaryRay(), width, height, dx + jitterX, dy + jitterY - 0.3333333);
    RenderEngine::traceLevel() = 0;
    RenderEngine::trace(RenderEngine::primaryRay(), &colour);
    Color::clipColor(&colour, &colour);
    Color::scaleColor(&colour, &colour, 0.11111111);
    Color::addColor(result, result, &colour);
    jittOffset += 10;

    jitterX =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    RenderFrame::createRay(
        RenderEngine::primaryRay(), width, height, dx + jitterX, dy + jitterY + 0.3333333);
    RenderEngine::traceLevel() = 0;
    RenderEngine::trace(RenderEngine::primaryRay(), &colour);
    Color::clipColor(&colour, &colour);
    Color::scaleColor(&colour, &colour, 0.11111111);
    Color::addColor(result, result, &colour);
    jittOffset += 10;

    jitterX =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    RenderFrame::createRay(RenderEngine::primaryRay(), width, height, dx + jitterX + 0.3333333,
        dy + jitterY - 0.3333333);
    RenderEngine::traceLevel() = 0;
    RenderEngine::trace(RenderEngine::primaryRay(), &colour);
    Color::clipColor(&colour, &colour);
    Color::scaleColor(&colour, &colour, 0.11111111);
    Color::addColor(result, result, &colour);
    jittOffset += 10;

    jitterX =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    RenderFrame::createRay(
        RenderEngine::primaryRay(), width, height, dx + jitterX + 0.3333333, dy + jitterY);
    RenderEngine::traceLevel() = 0;
    RenderEngine::trace(RenderEngine::primaryRay(), &colour);
    Color::clipColor(&colour, &colour);
    Color::scaleColor(&colour, &colour, 0.11111111);
    Color::addColor(result, result, &colour);
    jittOffset += 10;

    jitterX =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    RenderFrame::createRay(RenderEngine::primaryRay(), width, height, dx + jitterX + 0.3333333,
        dy + jitterY + 0.3333333);
    RenderEngine::traceLevel() = 0;
    RenderEngine::trace(RenderEngine::primaryRay(), &colour);
    Color::clipColor(&colour, &colour);
    Color::scaleColor(&colour, &colour, 0.11111111);
    Color::addColor(result, result, &colour);

    if ((y != globalRenderingConfiguration.firstLine - 1) && (globalRenderingConfiguration.options & DISPLAY)) {
    }
}

void
RenderEngine::readRenderedPart()
{
    int rc;
    int lineNumber;
    while ((rc = globalRenderingConfiguration.outputFileInputStream->readLine(
                previousLine, &lineNumber)) == 1) {
    }

    globalRenderingConfiguration.firstLine = lineNumber + 1;

    if (rc == 0) {
        globalRenderingConfiguration.outputFileInputStream->close();
        if (globalRenderingConfiguration.outputFileInputStream->open(
                globalRenderingConfiguration.outputFileName,
                &RenderEngine::renderFrame().screenWidth, &RenderEngine::renderFrame().screenHeight,
                globalRenderingConfiguration.fileBufferSize, RenderOutput::APPEND_MODE) != 1) {
            Logger::error("Error opening output file\n");
            exit(1);
        }
        return;
    }

    Logger::error("Error reading aborted data file\n");
}

void
RenderEngine::startTracing()
{
    RGBAColor colour;
    int x;
    int y;
    for (y = (globalRenderingConfiguration.options & ANTIALIAS) ? globalRenderingConfiguration.firstLine - 1 : globalRenderingConfiguration.firstLine; y < globalRenderingConfiguration.lastLine;
        y++) {

        RenderFrame::checkStats(y);

        for (x = 0; x < RenderEngine::renderFrame().screenWidth; x++) {

            if (RenderEngine::stopFlag()) {
                if (globalRenderingConfiguration.outputFileInputStream != nullptr) {
                    globalRenderingConfiguration.outputFileInputStream->close();
                }
                /* exit with error if image not completed/user abort*/
                exit(2);
            }

            globalStatistics.numberOfPixels++;

            RenderFrame::createRay(RenderEngine::primaryRay(), RenderEngine::renderFrame().screenWidth,
                RenderEngine::renderFrame().screenHeight, (double)x, (double)y);
            RenderEngine::traceLevel() = 0;
            RenderEngine::trace(&ray, &colour);
            Color::clipColor(&colour, &colour);

            currentLine[x] = colour;

            if (globalRenderingConfiguration.options & ANTIALIAS) {
                RenderFrame::doAntiAliasing(x, y, &colour);
            }

            if (y != globalRenderingConfiguration.firstLine - 1) {
                if (globalRenderingConfiguration.options & DISPLAY) {
                    (void)x;
                    (void)y;
                }
            }
        }
        RenderFrame::outputLine(y);
    }

    if (globalRenderingConfiguration.options & DISKWRITE) {
        globalRenderingConfiguration.outputFileInputStream->writeLine(
            previousLine, globalRenderingConfiguration.lastLine - 1);
    }
}

void
RenderFrame::checkStats(int y)
{
    /* New verbose options CdW */
    if (globalRenderingConfiguration.options & VERBOSE && globalRenderingConfiguration.verboseFormat == '0') {
        Logger::info("POV-Ray rendering %s to %s", globalRenderingConfiguration.inputFileName, globalRenderingConfiguration.outputFileName);
        if ((globalRenderingConfiguration.firstLine != 0) || (globalRenderingConfiguration.lastLine != RenderEngine::renderFrame().screenHeight)) {
            Logger::info(" from %4d to %4d:\n", globalRenderingConfiguration.firstLine, globalRenderingConfiguration.lastLine);
        } else {
            Logger::info(":\n");
        }
        Logger::info("Res %4d X %4d. Calc line %4d of %4d", RenderEngine::renderFrame().screenWidth,
            RenderEngine::renderFrame().screenHeight, (y - globalRenderingConfiguration.firstLine) + 1,
            globalRenderingConfiguration.lastLine - globalRenderingConfiguration.firstLine);
        if (!(globalRenderingConfiguration.options & ANTIALIAS)) {
            Logger::info(".");
        }
    }
    if (globalRenderingConfiguration.options & VERBOSE_FILE) {
        java::FileOutputStream statFile(globalRenderingConfiguration.statFileName);
        char buf[32];
        snprintf(buf, sizeof(buf), "Line %4d.\n", y);
        for (int i = 0; buf[i] != '\0'; i++) {
            statFile.write((unsigned char)buf[i]);
        }
        statFile.close();
    }

    /* Use -vO for Old style verbose */
    if (globalRenderingConfiguration.options & VERBOSE && (globalRenderingConfiguration.verboseFormat == 'O')) {
        Logger::info("Line %4d", y);
    }
    if (globalRenderingConfiguration.options & VERBOSE && globalRenderingConfiguration.verboseFormat == '1') {
        fprintf(stderr, "Res %4d X %4d. Calc line %4d of %4d",
            RenderEngine::renderFrame().screenWidth, RenderEngine::renderFrame().screenHeight,
            (y - globalRenderingConfiguration.firstLine) + 1, globalRenderingConfiguration.lastLine - globalRenderingConfiguration.firstLine);
        if (!(globalRenderingConfiguration.options & ANTIALIAS)) {
            fprintf(stderr, ".");
        }
    }

    if (globalRenderingConfiguration.options & ANTIALIAS) {
        superSampleCount = 0;
    }
}

void
RenderFrame::doAntiAliasing(int x, int y, RGBAColor *colour)
{
    char antialiasCenterFlag = 0;

    currentLineAntialiasedFlags[x] = 0;

    if (x != 0) {
        if (Color::colorDistance(&currentLine[x - 1], &currentLine[x]) >=
            RenderEngine::renderFrame().antialiasThreshold) {
            antialiasCenterFlag = 1;
            if (!(currentLineAntialiasedFlags[x - 1])) {
                RenderEngine::supersample(&currentLine[x - 1], x - 1, y,
                    RenderEngine::renderFrame().screenWidth, RenderEngine::renderFrame().screenHeight);
                currentLineAntialiasedFlags[x - 1] = 1;
                superSampleCount++;
            }
        }
    }

    if (y != globalRenderingConfiguration.firstLine - 1) {
        if (Color::colorDistance(&previousLine[x], &currentLine[x]) >=
            RenderEngine::renderFrame().antialiasThreshold) {
            antialiasCenterFlag = 1;
            if (!(previousLineAntialiasedFlags[x])) {
                RenderEngine::supersample(&previousLine[x], x, y - 1,
                    RenderEngine::renderFrame().screenWidth, RenderEngine::renderFrame().screenHeight);
                previousLineAntialiasedFlags[x] = 1;
                superSampleCount++;
            }
        }
    }

    if (antialiasCenterFlag) {
        RenderEngine::supersample(&currentLine[x], x, y,
            RenderEngine::renderFrame().screenWidth, RenderEngine::renderFrame().screenHeight);
        currentLineAntialiasedFlags[x] = 1;
        *colour = currentLine[x];
        superSampleCount++;
    }
}

void
RenderEngine::initializeRenderer()
{
    int i;

    RenderEngine::primaryRay() = &ray;
    previousLine = new RGBAColor[(RenderEngine::renderFrame().screenWidth + 1)];
    currentLine = new RGBAColor[(RenderEngine::renderFrame().screenWidth + 1)];

    for (i = 0; i <= RenderEngine::renderFrame().screenWidth; i++) {
        previousLine[i].Red = 0.0;
        previousLine[i].Green = 0.0;
        previousLine[i].Blue = 0.0;

        currentLine[i].Red = 0.0;
        currentLine[i].Green = 0.0;
        currentLine[i].Blue = 0.0;
    }

    if (globalRenderingConfiguration.options & ANTIALIAS) {
        previousLineAntialiasedFlags = new char[(RenderEngine::renderFrame().screenWidth + 1)];
        currentLineAntialiasedFlags = new char[(RenderEngine::renderFrame().screenWidth + 1)];

        for (i = 0; i <= RenderEngine::renderFrame().screenWidth; i++) {
            (previousLineAntialiasedFlags)[i] = 0;
            (currentLineAntialiasedFlags)[i] = 0;
        }
    }

    ray.position = RenderEngine::renderFrame().viewPoint.Location;
}

void
RenderFrame::outputLine(int y)
{
    RGBAColor *tempColourPtr;
    char *tempCharPtr;

    if (globalRenderingConfiguration.options & DISKWRITE) {
        if (y > globalRenderingConfiguration.firstLine) {
            globalRenderingConfiguration.outputFileInputStream->writeLine(previousLine, y - 1);
        }
    }

    if (globalRenderingConfiguration.options & VERBOSE) {
        if (globalRenderingConfiguration.options & ANTIALIAS && globalRenderingConfiguration.verboseFormat != '1') {
            Logger::info(" supersampled %d times.", superSampleCount);
        }

        if (globalRenderingConfiguration.options & ANTIALIAS && globalRenderingConfiguration.verboseFormat == '1') {
            fprintf(stderr, " supersampled %d times.", superSampleCount);
        }
        if (globalRenderingConfiguration.verboseFormat == '1') {
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
RenderEngine::trace(RayWithSegments *ray, RGBAColor *colour)
{
    SimpleBody *object;
    Intersection *localIntersection;
    Intersection *newIntersection;
    int intersectionFound;

    globalStatistics.numberOfRays++;
    Color::makeColor(colour, 0.0, 0.0, 0.0);

    intersectionFound = FALSE;
    localIntersection = nullptr;

    if (RenderEngine::traceLevel() > (int)RenderEngine::maxTraceLevel()) {
        return;
    }

    if (RenderEngine::renderFrame().fogDistance == 0.0) {
        Color::makeColor(colour, 0.0, 0.0, 0.0);
    } else {
        *colour = RenderEngine::renderFrame().fogColour;
    }

    if (globalRenderingConfiguration.options & DEBUGGING) {
        Logger::info("Calculating intersections level %d\n", RenderEngine::traceLevel());
    }

    /* What objects does this ray intersect? */
    for (object = RenderEngine::renderFrame().Objects; object != nullptr;
        object = object->nextObject) {
        if ((newIntersection = GeometryOperations::intersect(object, ray)) !=
            nullptr) {
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
        RayShaderPipeline::shadeSurface(
            localIntersection, colour, ray, FALSE, getTraceService());
        delete localIntersection;
    }
}

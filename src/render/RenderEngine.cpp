/**
This module implements the main raytracing loop.

01/16/92 dfm     Added David Buck's bug fix to add a different offset
                     to x and y coordinates for each call to Rand3D() in the
                     subsampling routine.  Said to smooth the anti-aliasing.
                     Previously, each call returned the same random number,
                     hence, no true jittering.
                     I consider this an interim fix until we get a better
                     algorithm for anti-aliasing.
*/

#include <cstdio>
#include <new>

#include "java/io/FileOutputStream.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"
#include "common/statistics/Statistics.h"
#include "common/dataStructures/PriorityQueuePool.txx"
#include "common/RenderRuntimeState.h"
#include "environment/material/RenderOutput.h"
#include "environment/material/RendererConfiguration.h"
#include "render/ColorOperations.h"
#include "render/RayShaderPipeline.h"
#include "render/RenderEngine.h"
#include "render/SceneDump.h"
#include "render/shaders/TraceService.h"

RenderEngine *RenderEngine::sActive = nullptr;

RenderEngine::~RenderEngine()
{
    delete[] mPreviousLine;
    delete[] mCurrentLine;
    delete[] mPreviousLineAntialiasedFlags;
    delete[] mCurrentLineAntialiasedFlags;
}

const RenderingConfiguration &
RenderEngine::getActiveConfig()
{
    return sActive->mContext->getConfig();
}

RenderingConfiguration &
RenderEngine::getActiveMutableConfig()
{
    return const_cast<RenderingConfiguration &>(getActiveConfig());
}

Statistics &
RenderEngine::getActiveStatistics()
{
    return sActive->mContext->getStatistics();
}

TextureUtils &
RenderEngine::getActiveTextureUtils()
{
    return sActive->mContext->getTextureUtils();
}

IntersectionPriorityQueuePool &
RenderEngine::getActiveIntersectionQueuePool()
{
    return sActive->mIntersectionQueuePool;
}

inline unsigned short
RenderEngine::rand3dInline(int a, int b)
{
    ProceduralNoise &noise = RenderEngine::getActiveTextureUtils().getProceduralNoise();
    return noise.checksumTable().eval((int)(noise.hashTable()[(int)(noise.hashTable()[(int)(a & 0xfff)] ^ b) &
                                  0xfff]));
}

namespace {
ColorRgba *
allocateColorBuffer(int count)
{
    ColorRgba *buffer = static_cast<ColorRgba *>(
        ::operator new[](sizeof(ColorRgba) * count));
    for (int i = 0; i < count; i++) {
        new (&buffer[i]) ColorRgba(0.0, 0.0, 0.0, 0.0);
    }
    return buffer;
}
}

Scene &
RenderEngine::scene()
{
    return *sActive->mScene;
}

RayWithSegments *&
RenderEngine::primaryRay()
{
    return sActive->mPrimaryRay;
}

int &
RenderEngine::traceLevel()
{
    return sActive->mTraceLevel;
}

double &
RenderEngine::maxTraceLevel()
{
    return sActive->mContext->getRuntime().getMaxTraceLevel();
}

volatile int &
RenderEngine::stopFlag()
{
    return sActive->mContext->getRuntime().getStopFlag();
}

void
RenderEngine::traceServiceTrace(void *context, RayWithSegments *ray, ColorRgba *color)
{
    (void)context;
    RenderEngine::trace(ray, color);
}

void
RenderEngine::traceServiceShadeShadow(
    void *context, Intersection *intersection, ColorRgba *color)
{
    RenderEngine *engine = static_cast<RenderEngine *>(context);
    RayShaderPipeline::shadeSurface(
        intersection, color, nullptr, true, &engine->mTraceService,
        &engine->mContext->getTextureUtils());
}

const TraceService *
RenderEngine::getTraceService()
{
    return &sActive->mTraceService;
}

void
Scene::createRay(
    RayWithSegments *ray, int width, int height, double x, double y)
{
    double xScalar;
    double yScalar;
    Vector3Dd tempVect1;
    Vector3Dd tempVect2;

    // Convert the X Coordinate to be a double from 0.0 to 1.0
    xScalar = (x - (double)width / 2.0) / (double)width;

    // Convert the Y Coordinate to be a double from 0.0 to 1.0
    yScalar =
        (((double)(RenderEngine::scene().getScreenHeight() - 1) - y) - (double)height / 2.0) /
        (double)height;

    const Camera &viewPoint = RenderEngine::scene().getViewPoint();
    tempVect1 = viewPoint.getUp().multiply(yScalar);
    tempVect2 = viewPoint.getRight().multiply(xScalar);
    ray->setDirection(tempVect1.add(tempVect2));
    ray->setDirection(ray->getDirection().add(viewPoint.getDirection()));
    ray->setDirection(ray->getDirection().normalizedFast());
    ray->initializeContainers();
    ray->setPrimaryRay(true);
    ray->setQuadricConstantsCached(false);
    RenderContext *ctx = RenderEngine::getActiveContext();
    if (ctx) {
        ray->setStatistics(&ctx->getStatistics());
        ray->setConfig(&ctx->getConfig());
        ray->setIntersectionQueuePool(&RenderEngine::getActiveIntersectionQueuePool());
    }
}

void
RenderEngine::supersample(
    ColorRgba *result, int x, int y, int width, int height)
{
    ColorRgba color(0.0, 0.0, 0.0, 0.0);
    double dx;
    double dy;
    double jitterX;
    double jitterY;
    int jittOffset;

    dx = (double)x;
    dy = (double)y;
    jittOffset = 10;

    RenderEngine::getActiveStatistics().incrementNumberOfPixelsSupersampled();

    result->setR(0.0); result->setG(0.0); result->setB(0.0); result->setA(0);

    jitterX = (RenderEngine::rand3dInline(x + jittOffset, y) & 0x7FFF) /
                  32768.0 * 0.33333333 -
              0.16666666;
    jitterY = (RenderEngine::rand3dInline(x + jittOffset, y) & 0x7FFF) /
                  32768.0 * 0.33333333 -
              0.16666666;
    Scene::createRay(RenderEngine::primaryRay(), RenderEngine::scene().getScreenWidth(),
        RenderEngine::scene().getScreenHeight(), dx + jitterX, dy + jitterY);

    RenderEngine::traceLevel() = 0;
    RenderEngine::trace(RenderEngine::primaryRay(), &color);
    ColorOperations::clipColor(&color, &color);
    ColorOperations::scaleColor(&color, &color, 0.11111111);
    ColorOperations::addColor(result, result, &color);
    jittOffset += 10;

    jitterX =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    Scene::createRay(RenderEngine::primaryRay(), width, height, dx + jitterX - 0.3333333,
        dy + jitterY - 0.3333333);
    RenderEngine::traceLevel() = 0;
    RenderEngine::trace(RenderEngine::primaryRay(), &color);
    ColorOperations::clipColor(&color, &color);
    ColorOperations::scaleColor(&color, &color, 0.11111111);
    ColorOperations::addColor(result, result, &color);
    jittOffset += 10;

    jitterX =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    Scene::createRay(
        RenderEngine::primaryRay(), width, height, dx + jitterX - 0.3333333, dy + jitterY);
    RenderEngine::traceLevel() = 0;
    RenderEngine::trace(RenderEngine::primaryRay(), &color);
    ColorOperations::clipColor(&color, &color);
    ColorOperations::scaleColor(&color, &color, 0.11111111);
    ColorOperations::addColor(result, result, &color);
    jittOffset += 10;

    jitterX =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    Scene::createRay(RenderEngine::primaryRay(), width, height, dx + jitterX - 0.3333333,
        dy + jitterY + 0.3333333);
    RenderEngine::traceLevel() = 0;
    RenderEngine::trace(RenderEngine::primaryRay(), &color);
    ColorOperations::clipColor(&color, &color);
    ColorOperations::scaleColor(&color, &color, 0.11111111);
    ColorOperations::addColor(result, result, &color);
    jittOffset += 10;

    jitterX =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    Scene::createRay(
        RenderEngine::primaryRay(), width, height, dx + jitterX, dy + jitterY - 0.3333333);
    RenderEngine::traceLevel() = 0;
    RenderEngine::trace(RenderEngine::primaryRay(), &color);
    ColorOperations::clipColor(&color, &color);
    ColorOperations::scaleColor(&color, &color, 0.11111111);
    ColorOperations::addColor(result, result, &color);
    jittOffset += 10;

    jitterX =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    Scene::createRay(
        RenderEngine::primaryRay(), width, height, dx + jitterX, dy + jitterY + 0.3333333);
    RenderEngine::traceLevel() = 0;
    RenderEngine::trace(RenderEngine::primaryRay(), &color);
    ColorOperations::clipColor(&color, &color);
    ColorOperations::scaleColor(&color, &color, 0.11111111);
    ColorOperations::addColor(result, result, &color);
    jittOffset += 10;

    jitterX =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    Scene::createRay(RenderEngine::primaryRay(), width, height, dx + jitterX + 0.3333333,
        dy + jitterY - 0.3333333);
    RenderEngine::traceLevel() = 0;
    RenderEngine::trace(RenderEngine::primaryRay(), &color);
    ColorOperations::clipColor(&color, &color);
    ColorOperations::scaleColor(&color, &color, 0.11111111);
    ColorOperations::addColor(result, result, &color);
    jittOffset += 10;

    jitterX =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    Scene::createRay(
        RenderEngine::primaryRay(), width, height, dx + jitterX + 0.3333333, dy + jitterY);
    RenderEngine::traceLevel() = 0;
    RenderEngine::trace(RenderEngine::primaryRay(), &color);
    ColorOperations::clipColor(&color, &color);
    ColorOperations::scaleColor(&color, &color, 0.11111111);
    ColorOperations::addColor(result, result, &color);
    jittOffset += 10;

    jitterX =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (RenderEngine::rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    Scene::createRay(RenderEngine::primaryRay(), width, height, dx + jitterX + 0.3333333,
        dy + jitterY + 0.3333333);
    RenderEngine::traceLevel() = 0;
    RenderEngine::trace(RenderEngine::primaryRay(), &color);
    ColorOperations::clipColor(&color, &color);
    ColorOperations::scaleColor(&color, &color, 0.11111111);
    ColorOperations::addColor(result, result, &color);

    if ((y != RenderEngine::getActiveConfig().getFirstLine() - 1) &&
        RenderEngine::getActiveConfig().hasOptionFlags(RenderingConfiguration::DISPLAY)) {
    }
}

void
RenderEngine::readRenderedPart()
{
    int rc;
    int lineNumber;
    while ((rc = RenderEngine::getActiveConfig().getOutputFileInputStream()->readLine(
                RenderEngine::sActive->mPreviousLine, &lineNumber)) == 1) {
    }

    RenderEngine::getActiveMutableConfig().setFirstLine(lineNumber + 1);

    if (rc == 0) {
        RenderEngine::getActiveConfig().getOutputFileInputStream()->close();
        if (RenderEngine::getActiveConfig().getOutputFileInputStream()->open(
                RenderEngine::getActiveMutableConfig().getOutputFileNameBuffer(),
                &RenderEngine::scene().getScreenWidth(),
                &RenderEngine::scene().getScreenHeight(),
                RenderEngine::getActiveConfig().getFileBufferSize(), RenderOutput::APPEND_MODE,
                RenderEngine::getActiveConfig().getFirstLine()) != 1) {
            Logger::reportMessage("RenderEngine", Logger::FATAL_ERROR, "", "Error opening output file\n");
        }
        return;
    }

    Logger::reportMessage("RenderEngine", Logger::ERROR, "", "Error reading aborted data file\n");
}

void
RenderEngine::startTracing()
{
    const char *dumpEnv = getenv("POVCPP_DUMP_SCENE");
    if (dumpEnv != nullptr && dumpEnv[0] == '1') {
        SceneDumper::dumpSceneStructure(stderr);
    }

    ColorRgba color(0.0, 0.0, 0.0, 0.0);
    int x;
    int y;
    for (y = RenderEngine::getActiveConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS)
                ? RenderEngine::getActiveConfig().getFirstLine() - 1
                : RenderEngine::getActiveConfig().getFirstLine();
        y < RenderEngine::getActiveConfig().getLastLine();
        y++) {

        Scene::checkStats(y);

        for (x = 0; x < RenderEngine::scene().getScreenWidth(); x++) {

            if (RenderEngine::stopFlag()) {
                if (RenderEngine::getActiveConfig().getOutputFileInputStream() != nullptr) {
                    RenderEngine::getActiveConfig().getOutputFileInputStream()->close();
                }
                // Exit with error if image not completed/user abort
                exit(2);
            }

            RenderEngine::getActiveStatistics().incrementNumberOfPixels();

            Scene::createRay(RenderEngine::primaryRay(),
                RenderEngine::scene().getScreenWidth(),
                RenderEngine::scene().getScreenHeight(), (double)x, (double)y);
            RenderEngine::traceLevel() = 0;
            RenderEngine::trace(&RenderEngine::sActive->mRay, &color);
            ColorOperations::clipColor(&color, &color);

            RenderEngine::sActive->mCurrentLine[x] = color;

            if (RenderEngine::getActiveConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS)) {
                Scene::doAntiAliasing(x, y, &color);
            }

            if (y != RenderEngine::getActiveConfig().getFirstLine() - 1) {
                if (RenderEngine::getActiveConfig().hasOptionFlags(RenderingConfiguration::DISPLAY)) {
                    (void)x;
                    (void)y;
                }
            }
        }
        Scene::outputLine(y);
    }

    if (RenderEngine::getActiveConfig().hasOptionFlags(RenderingConfiguration::DISKWRITE)) {
        RenderEngine::getActiveConfig().getOutputFileInputStream()->writeLine(
            RenderEngine::sActive->mPreviousLine, RenderEngine::getActiveConfig().getLastLine() - 1);
    }
}

void
Scene::checkStats(int y)
{
    // New verbose options CdW
    if (RenderEngine::getActiveConfig().hasOptionFlags(RenderingConfiguration::VERBOSE) &&
        RenderEngine::getActiveConfig().getVerboseFormat() == '0') {
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "POV-Ray rendering %s to %s",
                RenderEngine::getActiveConfig().getInputFileName(),
                RenderEngine::getActiveConfig().getOutputFileName());
            Logger::reportMessage("RenderEngine", Logger::WARNING, "", _logMsg);
        }
        if ((RenderEngine::getActiveConfig().getFirstLine() != 0) ||
            (RenderEngine::getActiveConfig().getLastLine() !=
                RenderEngine::scene().getScreenHeight())) {
            {
                char _logMsg[1024];
                snprintf(_logMsg, sizeof(_logMsg), " from %4d to %4d:\n",
                    RenderEngine::getActiveConfig().getFirstLine(),
                    RenderEngine::getActiveConfig().getLastLine());
                Logger::reportMessage("RenderEngine", Logger::WARNING, "", _logMsg);
            }
        } else {
            Logger::reportMessage("RenderEngine", Logger::WARNING, "", ":\n");
        }
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "Res %4d X %4d. Calc line %4d of %4d",
                RenderEngine::scene().getScreenWidth(),
                RenderEngine::scene().getScreenHeight(),
                (y - RenderEngine::getActiveConfig().getFirstLine()) + 1,
                RenderEngine::getActiveConfig().getLastLine() - RenderEngine::getActiveConfig().getFirstLine());
            Logger::reportMessage("RenderEngine", Logger::WARNING, "", _logMsg);
        }
        if (!RenderEngine::getActiveConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS)) {
            Logger::reportMessage("RenderEngine", Logger::WARNING, "", ".");
        }
    }
    if (RenderEngine::getActiveConfig().hasOptionFlags(RenderingConfiguration::VERBOSE_FILE)) {
        java::FileOutputStream statFile(RenderEngine::getActiveConfig().getStatFileName());
        char buf[32];
        snprintf(buf, sizeof(buf), "Line %4d.\n", y);
        for (int i = 0; buf[i] != '\0'; i++) {
            statFile.write((unsigned char)buf[i]);
        }
        statFile.close();
    }

    // Use -vO for Old style verbose
    if (RenderEngine::getActiveConfig().hasOptionFlags(RenderingConfiguration::VERBOSE) &&
        (RenderEngine::getActiveConfig().getVerboseFormat() == 'O')) {
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "Line %4d", y);
            Logger::reportMessage("RenderEngine", Logger::WARNING, "", _logMsg);
        }
    }
    if (RenderEngine::getActiveConfig().hasOptionFlags(RenderingConfiguration::VERBOSE) &&
        RenderEngine::getActiveConfig().getVerboseFormat() == '1') {
        fprintf(stderr, "Res %4d X %4d. Calc line %4d of %4d",
            RenderEngine::scene().getScreenWidth(),
            RenderEngine::scene().getScreenHeight(),
            (y - RenderEngine::getActiveConfig().getFirstLine()) + 1,
            RenderEngine::getActiveConfig().getLastLine() - RenderEngine::getActiveConfig().getFirstLine());
        if (!RenderEngine::getActiveConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS)) {
            fprintf(stderr, ".");
        }
    }

    if (RenderEngine::getActiveConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS)) {
        RenderEngine::sActive->mSuperSampleCount = 0;
    }
}

void
Scene::doAntiAliasing(int x, int y, ColorRgba *color)
{
    char antialiasCenterFlag = 0;

    RenderEngine::sActive->mCurrentLineAntialiasedFlags[x] = 0;

    if (x != 0) {
        if (ColorOperations::colorDistance(&RenderEngine::sActive->mCurrentLine[x - 1], &RenderEngine::sActive->mCurrentLine[x]) >=
            RenderEngine::scene().getAntialiasThreshold()) {
            antialiasCenterFlag = 1;
            if (!(RenderEngine::sActive->mCurrentLineAntialiasedFlags[x - 1])) {
                RenderEngine::supersample(&RenderEngine::sActive->mCurrentLine[x - 1], x - 1, y,
                    RenderEngine::scene().getScreenWidth(),
                    RenderEngine::scene().getScreenHeight());
                RenderEngine::sActive->mCurrentLineAntialiasedFlags[x - 1] = 1;
                RenderEngine::sActive->mSuperSampleCount++;
            }
        }
    }

    if (y != RenderEngine::getActiveConfig().getFirstLine() - 1) {
        if (ColorOperations::colorDistance(&RenderEngine::sActive->mPreviousLine[x], &RenderEngine::sActive->mCurrentLine[x]) >=
            RenderEngine::scene().getAntialiasThreshold()) {
            antialiasCenterFlag = 1;
            if (!(RenderEngine::sActive->mPreviousLineAntialiasedFlags[x])) {
                RenderEngine::supersample(&RenderEngine::sActive->mPreviousLine[x], x, y - 1,
                    RenderEngine::scene().getScreenWidth(),
                    RenderEngine::scene().getScreenHeight());
                RenderEngine::sActive->mPreviousLineAntialiasedFlags[x] = 1;
                RenderEngine::sActive->mSuperSampleCount++;
            }
        }
    }

    if (antialiasCenterFlag) {
        RenderEngine::supersample(&RenderEngine::sActive->mCurrentLine[x], x, y,
            RenderEngine::scene().getScreenWidth(),
            RenderEngine::scene().getScreenHeight());
        RenderEngine::sActive->mCurrentLineAntialiasedFlags[x] = 1;
        *color = RenderEngine::sActive->mCurrentLine[x];
        RenderEngine::sActive->mSuperSampleCount++;
    }
}

void
RenderEngine::initializeRenderer()
{
    int i;

    RenderEngine::primaryRay() = &sActive->mRay;
    sActive->mPreviousLine = allocateColorBuffer(RenderEngine::scene().getScreenWidth() + 1);
    sActive->mCurrentLine = allocateColorBuffer(RenderEngine::scene().getScreenWidth() + 1);

    for (i = 0; i <= RenderEngine::scene().getScreenWidth(); i++) {
        sActive->mPreviousLine[i].setR(0.0);
        sActive->mPreviousLine[i].setG(0.0);
        sActive->mPreviousLine[i].setB(0.0);

        sActive->mCurrentLine[i].setR(0.0);
        sActive->mCurrentLine[i].setG(0.0);
        sActive->mCurrentLine[i].setB(0.0);
    }

    if (RenderEngine::getActiveConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS)) {
        sActive->mPreviousLineAntialiasedFlags =
            new char[(RenderEngine::scene().getScreenWidth() + 1)];
        sActive->mCurrentLineAntialiasedFlags =
            new char[(RenderEngine::scene().getScreenWidth() + 1)];

        for (i = 0; i <= RenderEngine::scene().getScreenWidth(); i++) {
            (sActive->mPreviousLineAntialiasedFlags)[i] = 0;
            (sActive->mCurrentLineAntialiasedFlags)[i] = 0;
        }
    }

    sActive->mRay.setOrigin(RenderEngine::scene().getViewPoint().getLocation());
}

void
Scene::outputLine(int y)
{
    ColorRgba *tempColorPtr;
    char *tempCharPtr;

    if (RenderEngine::getActiveConfig().hasOptionFlags(RenderingConfiguration::DISKWRITE)) {
        if (y > RenderEngine::getActiveConfig().getFirstLine()) {
            RenderEngine::getActiveConfig().getOutputFileInputStream()->writeLine(RenderEngine::sActive->mPreviousLine, y - 1);
        }
    }

    if (RenderEngine::getActiveConfig().hasOptionFlags(RenderingConfiguration::VERBOSE)) {
        if (RenderEngine::getActiveConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS) &&
            RenderEngine::getActiveConfig().getVerboseFormat() != '1') {
            {
                char _logMsg[1024];
                snprintf(_logMsg, sizeof(_logMsg), " supersampled %d times.", RenderEngine::sActive->mSuperSampleCount);
                Logger::reportMessage("RenderEngine", Logger::WARNING, "", _logMsg);
            }
        }

        if (RenderEngine::getActiveConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS) &&
            RenderEngine::getActiveConfig().getVerboseFormat() == '1') {
            fprintf(stderr, " supersampled %d times.", RenderEngine::sActive->mSuperSampleCount);
        }
        if (RenderEngine::getActiveConfig().getVerboseFormat() == '1') {
            fprintf(stderr, "\r");
        } else {
            fprintf(stderr, "\n");
        }
    }
    tempColorPtr = RenderEngine::sActive->mPreviousLine;
    RenderEngine::sActive->mPreviousLine = RenderEngine::sActive->mCurrentLine;
    RenderEngine::sActive->mCurrentLine = tempColorPtr;

    tempCharPtr = RenderEngine::sActive->mPreviousLineAntialiasedFlags;
    RenderEngine::sActive->mPreviousLineAntialiasedFlags = RenderEngine::sActive->mCurrentLineAntialiasedFlags;
    RenderEngine::sActive->mCurrentLineAntialiasedFlags = tempCharPtr;
}

void
RenderEngine::trace(RayWithSegments *ray, ColorRgba *color)
{
    BoundedGeometry *object;
    Intersection localIntersection;
    Intersection newIntersection;
    bool intersectionFound;

    RenderEngine::getActiveStatistics().incrementNumberOfRays();
    color->setR(0.0); color->setG(0.0); color->setB(0.0); color->setA(0);

    intersectionFound = false;

    if (RenderEngine::traceLevel() > (int)RenderEngine::maxTraceLevel()) {
        return;
    }

    if (RenderEngine::scene().getFogDistance() == 0.0) {
        color->setR(0.0); color->setG(0.0); color->setB(0.0); color->setA(0);
    } else {
        *color = RenderEngine::scene().getFogColor();
    }

    if (RenderEngine::getActiveConfig().hasOptionFlags(RenderingConfiguration::DEBUGGING)) {
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "Calculating intersections level %d\n", RenderEngine::traceLevel());
            Logger::reportMessage("RenderEngine", Logger::WARNING, "", _logMsg);
        }
    }

    // What objects does this ray intersect?
    const java::ArrayList<BoundedGeometry*> &sceneObjects =
        RenderEngine::scene().getObjects();
    for (long int i = sceneObjects.size() - 1; i >= 0; i--) {
        object = sceneObjects[i];
        if (object->intersect(ray, newIntersection)) {
            if (!intersectionFound || newIntersection.getDepth() < localIntersection.getDepth()) {
                localIntersection = newIntersection;
            }
            intersectionFound = true;
        }
    }

    if (intersectionFound) {
        RayShaderPipeline::shadeSurface(
            &localIntersection, color, ray, false, RenderEngine::getTraceService(),
            &RenderEngine::getActiveTextureUtils());
    }
}

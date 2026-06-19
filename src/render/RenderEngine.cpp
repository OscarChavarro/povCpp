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
#include "common/RenderRuntimeState.h"
#include "environment/material/RenderOutput.h"
#include "environment/material/RendererConfiguration.h"
#include "render/ColorOperations.h"
#include "render/RayShaderPipeline.h"
#include "render/RenderEngine.h"
#include "render/SceneDump.h"
#include "render/shaders/TraceService.h"

RenderEngine::~RenderEngine()
{
    delete[] mPreviousLine;
    delete[] mCurrentLine;
    delete[] mPreviousLineAntialiasedFlags;
    delete[] mCurrentLineAntialiasedFlags;
}

RenderingConfiguration &
RenderEngine::getMutableConfig()
{
    return const_cast<RenderingConfiguration &>(mContext->getConfig());
}

inline unsigned short
RenderEngine::rand3dInline(int a, int b)
{
    ProceduralNoise &noise = this->getTextureUtils().getProceduralNoise();
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

void
RenderEngine::traceServiceTrace(void *context, RayWithSegments *ray, ColorRgba *color)
{
    RenderEngine *engine = static_cast<RenderEngine *>(context);
    engine->trace(ray, color);
}

void
RenderEngine::traceServiceShadeShadow(
    void *context, Intersection *intersection, ColorRgba *color)
{
    RenderEngine *engine = static_cast<RenderEngine *>(context);
    RayShaderPipeline::shadeSurface(
        intersection, color, nullptr, true, &engine->mTraceService,
        &engine->mContext->getTextureUtils(), *engine->mContext, engine->mTraceLevel);
}

void
RenderEngine::createRay(
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
        (((double)(this->scene().getScreenHeight() - 1) - y) - (double)height / 2.0) /
        (double)height;

    const Camera &viewPoint = this->scene().getViewPoint();
    tempVect1 = viewPoint.getUp().multiply(yScalar);
    tempVect2 = viewPoint.getRight().multiply(xScalar);
    ray->setDirection(tempVect1.add(tempVect2));
    ray->setDirection(ray->getDirection().add(viewPoint.getDirection()));
    ray->setDirection(ray->getDirection().normalizedFast());
    ray->initializeContainers();
    ray->setPrimaryRay(true);
    ray->setQuadricConstantsCached(false);
    if (mContext) {
        ray->setStatistics(&mContext->getStatistics());
        ray->setConfig(&mContext->getConfig());
        ray->setIntersectionQueuePool(&mIntersectionQueuePool);
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

    this->getStatistics().incrementNumberOfPixelsSupersampled();

    result->setR(0.0); result->setG(0.0); result->setB(0.0); result->setA(0);

    jitterX = (this->rand3dInline(x + jittOffset, y) & 0x7FFF) /
                  32768.0 * 0.33333333 -
              0.16666666;
    jitterY = (this->rand3dInline(x + jittOffset, y) & 0x7FFF) /
                  32768.0 * 0.33333333 -
              0.16666666;
    this->createRay(this->primaryRay(), this->scene().getScreenWidth(),
        this->scene().getScreenHeight(), dx + jitterX, dy + jitterY);

    this->traceLevel() = 0;
    this->trace(this->primaryRay(), &color);
    ColorOperations::clipColor(&color, &color);
    ColorOperations::scaleColor(&color, &color, 0.11111111);
    ColorOperations::addColor(result, result, &color);
    jittOffset += 10;

    jitterX =
        (this->rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (this->rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    this->createRay(this->primaryRay(), width, height, dx + jitterX - 0.3333333,
        dy + jitterY - 0.3333333);
    this->traceLevel() = 0;
    this->trace(this->primaryRay(), &color);
    ColorOperations::clipColor(&color, &color);
    ColorOperations::scaleColor(&color, &color, 0.11111111);
    ColorOperations::addColor(result, result, &color);
    jittOffset += 10;

    jitterX =
        (this->rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (this->rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    this->createRay(
        this->primaryRay(), width, height, dx + jitterX - 0.3333333, dy + jitterY);
    this->traceLevel() = 0;
    this->trace(this->primaryRay(), &color);
    ColorOperations::clipColor(&color, &color);
    ColorOperations::scaleColor(&color, &color, 0.11111111);
    ColorOperations::addColor(result, result, &color);
    jittOffset += 10;

    jitterX =
        (this->rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (this->rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    this->createRay(this->primaryRay(), width, height, dx + jitterX - 0.3333333,
        dy + jitterY + 0.3333333);
    this->traceLevel() = 0;
    this->trace(this->primaryRay(), &color);
    ColorOperations::clipColor(&color, &color);
    ColorOperations::scaleColor(&color, &color, 0.11111111);
    ColorOperations::addColor(result, result, &color);
    jittOffset += 10;

    jitterX =
        (this->rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (this->rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    this->createRay(
        this->primaryRay(), width, height, dx + jitterX, dy + jitterY - 0.3333333);
    this->traceLevel() = 0;
    this->trace(this->primaryRay(), &color);
    ColorOperations::clipColor(&color, &color);
    ColorOperations::scaleColor(&color, &color, 0.11111111);
    ColorOperations::addColor(result, result, &color);
    jittOffset += 10;

    jitterX =
        (this->rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (this->rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    this->createRay(
        this->primaryRay(), width, height, dx + jitterX, dy + jitterY + 0.3333333);
    this->traceLevel() = 0;
    this->trace(this->primaryRay(), &color);
    ColorOperations::clipColor(&color, &color);
    ColorOperations::scaleColor(&color, &color, 0.11111111);
    ColorOperations::addColor(result, result, &color);
    jittOffset += 10;

    jitterX =
        (this->rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (this->rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    this->createRay(this->primaryRay(), width, height, dx + jitterX + 0.3333333,
        dy + jitterY - 0.3333333);
    this->traceLevel() = 0;
    this->trace(this->primaryRay(), &color);
    ColorOperations::clipColor(&color, &color);
    ColorOperations::scaleColor(&color, &color, 0.11111111);
    ColorOperations::addColor(result, result, &color);
    jittOffset += 10;

    jitterX =
        (this->rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (this->rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    this->createRay(
        this->primaryRay(), width, height, dx + jitterX + 0.3333333, dy + jitterY);
    this->traceLevel() = 0;
    this->trace(this->primaryRay(), &color);
    ColorOperations::clipColor(&color, &color);
    ColorOperations::scaleColor(&color, &color, 0.11111111);
    ColorOperations::addColor(result, result, &color);
    jittOffset += 10;

    jitterX =
        (this->rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    jitterY =
        (this->rand3dInline(x + jittOffset, y + jittOffset) & 0x7FFF) /
            32768.0 * 0.33333333 -
        0.16666666;
    this->createRay(this->primaryRay(), width, height, dx + jitterX + 0.3333333,
        dy + jitterY + 0.3333333);
    this->traceLevel() = 0;
    this->trace(this->primaryRay(), &color);
    ColorOperations::clipColor(&color, &color);
    ColorOperations::scaleColor(&color, &color, 0.11111111);
    ColorOperations::addColor(result, result, &color);

    if ((y != this->getConfig().getFirstLine() - 1) &&
        this->getConfig().hasOptionFlags(RenderingConfiguration::DISPLAY)) {
    }
}

void
RenderEngine::readRenderedPart()
{
    int rc;
    int lineNumber;
    while ((rc = this->getConfig().getOutputFileInputStream()->readLine(
                mPreviousLine, &lineNumber)) == 1) {
    }

    this->getMutableConfig().setFirstLine(lineNumber + 1);

    if (rc == 0) {
        this->getConfig().getOutputFileInputStream()->close();
        if (this->getConfig().getOutputFileInputStream()->open(
                this->getMutableConfig().getOutputFileNameBuffer(),
                &this->scene().getScreenWidth(),
                &this->scene().getScreenHeight(),
                this->getConfig().getFileBufferSize(), RenderOutput::APPEND_MODE,
                this->getConfig().getFirstLine()) != 1) {
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
        SceneDumper::dumpSceneStructure(stderr, this->scene());
    }

    ColorRgba color(0.0, 0.0, 0.0, 0.0);
    int x;
    int y;
    for (y = this->getConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS)
                ? this->getConfig().getFirstLine() - 1
                : this->getConfig().getFirstLine();
        y < this->getConfig().getLastLine();
        y++) {

        this->checkStats(y);

        for (x = 0; x < this->scene().getScreenWidth(); x++) {

            if (this->stopFlag()) {
                if (this->getConfig().getOutputFileInputStream() != nullptr) {
                    this->getConfig().getOutputFileInputStream()->close();
                }
                // Exit with error if image not completed/user abort
                exit(2);
            }

            this->getStatistics().incrementNumberOfPixels();

            this->createRay(this->primaryRay(),
                this->scene().getScreenWidth(),
                this->scene().getScreenHeight(), (double)x, (double)y);
            this->traceLevel() = 0;
            this->trace(&mRay, &color);
            ColorOperations::clipColor(&color, &color);

            mCurrentLine[x] = color;

            if (this->getConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS)) {
                this->doAntiAliasing(x, y, &color);
            }

            if (y != this->getConfig().getFirstLine() - 1) {
                if (this->getConfig().hasOptionFlags(RenderingConfiguration::DISPLAY)) {
                    (void)x;
                    (void)y;
                }
            }
        }
        this->outputLine(y);
    }

    if (this->getConfig().hasOptionFlags(RenderingConfiguration::DISKWRITE)) {
        this->getConfig().getOutputFileInputStream()->writeLine(
            mPreviousLine, this->getConfig().getLastLine() - 1);
    }
}

void
RenderEngine::checkStats(int y)
{
    // New verbose options CdW
    if (this->getConfig().hasOptionFlags(RenderingConfiguration::VERBOSE) &&
        this->getConfig().getVerboseFormat() == '0') {
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "POV-Ray rendering %s to %s",
                this->getConfig().getInputFileName(),
                this->getConfig().getOutputFileName());
            Logger::reportMessage("RenderEngine", Logger::WARNING, "", _logMsg);
        }
        if ((this->getConfig().getFirstLine() != 0) ||
            (this->getConfig().getLastLine() !=
                this->scene().getScreenHeight())) {
            {
                char _logMsg[1024];
                snprintf(_logMsg, sizeof(_logMsg), " from %4d to %4d:\n",
                    this->getConfig().getFirstLine(),
                    this->getConfig().getLastLine());
                Logger::reportMessage("RenderEngine", Logger::WARNING, "", _logMsg);
            }
        } else {
            Logger::reportMessage("RenderEngine", Logger::WARNING, "", ":\n");
        }
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "Res %4d X %4d. Calc line %4d of %4d",
                this->scene().getScreenWidth(),
                this->scene().getScreenHeight(),
                (y - this->getConfig().getFirstLine()) + 1,
                this->getConfig().getLastLine() - this->getConfig().getFirstLine());
            Logger::reportMessage("RenderEngine", Logger::WARNING, "", _logMsg);
        }
        if (!this->getConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS)) {
            Logger::reportMessage("RenderEngine", Logger::WARNING, "", ".");
        }
    }
    if (this->getConfig().hasOptionFlags(RenderingConfiguration::VERBOSE_FILE)) {
        java::FileOutputStream statFile(this->getConfig().getStatFileName());
        char buf[32];
        snprintf(buf, sizeof(buf), "Line %4d.\n", y);
        for (int i = 0; buf[i] != '\0'; i++) {
            statFile.write((unsigned char)buf[i]);
        }
        statFile.close();
    }

    // Use -vO for Old style verbose
    if (this->getConfig().hasOptionFlags(RenderingConfiguration::VERBOSE) &&
        (this->getConfig().getVerboseFormat() == 'O')) {
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "Line %4d", y);
            Logger::reportMessage("RenderEngine", Logger::WARNING, "", _logMsg);
        }
    }
    if (this->getConfig().hasOptionFlags(RenderingConfiguration::VERBOSE) &&
        this->getConfig().getVerboseFormat() == '1') {
        fprintf(stderr, "Res %4d X %4d. Calc line %4d of %4d",
            this->scene().getScreenWidth(),
            this->scene().getScreenHeight(),
            (y - this->getConfig().getFirstLine()) + 1,
            this->getConfig().getLastLine() - this->getConfig().getFirstLine());
        if (!this->getConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS)) {
            fprintf(stderr, ".");
        }
    }

    if (this->getConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS)) {
        mSuperSampleCount = 0;
    }
}

void
RenderEngine::doAntiAliasing(int x, int y, ColorRgba *color)
{
    char antialiasCenterFlag = 0;

    mCurrentLineAntialiasedFlags[x] = 0;

    if (x != 0) {
        if (ColorOperations::colorDistance(&mCurrentLine[x - 1], &mCurrentLine[x]) >=
            this->scene().getAntialiasThreshold()) {
            antialiasCenterFlag = 1;
            if (!(mCurrentLineAntialiasedFlags[x - 1])) {
                this->supersample(&mCurrentLine[x - 1], x - 1, y,
                    this->scene().getScreenWidth(),
                    this->scene().getScreenHeight());
                mCurrentLineAntialiasedFlags[x - 1] = 1;
                mSuperSampleCount++;
            }
        }
    }

    if (y != this->getConfig().getFirstLine() - 1) {
        if (ColorOperations::colorDistance(&mPreviousLine[x], &mCurrentLine[x]) >=
            this->scene().getAntialiasThreshold()) {
            antialiasCenterFlag = 1;
            if (!(mPreviousLineAntialiasedFlags[x])) {
                this->supersample(&mPreviousLine[x], x, y - 1,
                    this->scene().getScreenWidth(),
                    this->scene().getScreenHeight());
                mPreviousLineAntialiasedFlags[x] = 1;
                mSuperSampleCount++;
            }
        }
    }

    if (antialiasCenterFlag) {
        this->supersample(&mCurrentLine[x], x, y,
            this->scene().getScreenWidth(),
            this->scene().getScreenHeight());
        mCurrentLineAntialiasedFlags[x] = 1;
        *color = mCurrentLine[x];
        mSuperSampleCount++;
    }
}

void
RenderEngine::initializeRenderer()
{
    int i;

    this->primaryRay() = &mRay;
    mPreviousLine = allocateColorBuffer(this->scene().getScreenWidth() + 1);
    mCurrentLine = allocateColorBuffer(this->scene().getScreenWidth() + 1);

    for (i = 0; i <= this->scene().getScreenWidth(); i++) {
        mPreviousLine[i].setR(0.0);
        mPreviousLine[i].setG(0.0);
        mPreviousLine[i].setB(0.0);

        mCurrentLine[i].setR(0.0);
        mCurrentLine[i].setG(0.0);
        mCurrentLine[i].setB(0.0);
    }

    if (this->getConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS)) {
        mPreviousLineAntialiasedFlags =
            new char[(this->scene().getScreenWidth() + 1)];
        mCurrentLineAntialiasedFlags =
            new char[(this->scene().getScreenWidth() + 1)];

        for (i = 0; i <= this->scene().getScreenWidth(); i++) {
            (mPreviousLineAntialiasedFlags)[i] = 0;
            (mCurrentLineAntialiasedFlags)[i] = 0;
        }
    }

    mRay.setOrigin(this->scene().getViewPoint().getLocation());
}

void
RenderEngine::outputLine(int y)
{
    ColorRgba *tempColorPtr;
    char *tempCharPtr;

    if (this->getConfig().hasOptionFlags(RenderingConfiguration::DISKWRITE)) {
        if (y > this->getConfig().getFirstLine()) {
            this->getConfig().getOutputFileInputStream()->writeLine(mPreviousLine, y - 1);
        }
    }

    if (this->getConfig().hasOptionFlags(RenderingConfiguration::VERBOSE)) {
        if (this->getConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS) &&
            this->getConfig().getVerboseFormat() != '1') {
            {
                char _logMsg[1024];
                snprintf(_logMsg, sizeof(_logMsg), " supersampled %d times.", mSuperSampleCount);
                Logger::reportMessage("RenderEngine", Logger::WARNING, "", _logMsg);
            }
        }

        if (this->getConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS) &&
            this->getConfig().getVerboseFormat() == '1') {
            fprintf(stderr, " supersampled %d times.", mSuperSampleCount);
        }
        if (this->getConfig().getVerboseFormat() == '1') {
            fprintf(stderr, "\r");
        } else {
            fprintf(stderr, "\n");
        }
    }
    tempColorPtr = mPreviousLine;
    mPreviousLine = mCurrentLine;
    mCurrentLine = tempColorPtr;

    tempCharPtr = mPreviousLineAntialiasedFlags;
    mPreviousLineAntialiasedFlags = mCurrentLineAntialiasedFlags;
    mCurrentLineAntialiasedFlags = tempCharPtr;
}

void
RenderEngine::trace(RayWithSegments *ray, ColorRgba *color)
{
    BoundedGeometry *object;
    Intersection localIntersection;
    Intersection newIntersection;
    bool intersectionFound;

    this->getStatistics().incrementNumberOfRays();
    color->setR(0.0); color->setG(0.0); color->setB(0.0); color->setA(0);

    intersectionFound = false;

    if (this->traceLevel() > (int)this->maxTraceLevel()) {
        return;
    }

    if (this->scene().getFogDistance() == 0.0) {
        color->setR(0.0); color->setG(0.0); color->setB(0.0); color->setA(0);
    } else {
        *color = this->scene().getFogColor();
    }

    if (this->getConfig().hasOptionFlags(RenderingConfiguration::DEBUGGING)) {
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "Calculating intersections level %d\n", this->traceLevel());
            Logger::reportMessage("RenderEngine", Logger::WARNING, "", _logMsg);
        }
    }

    // What objects does this ray intersect?
    const java::ArrayList<BoundedGeometry*> &sceneObjects =
        this->scene().getObjects();
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
            &localIntersection, color, ray, false, this->getTraceService(),
            &this->getTextureUtils(), *mContext, mTraceLevel);
    }
}

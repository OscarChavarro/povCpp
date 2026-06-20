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
    delete[] previousLine;
    delete[] currentLine;
    delete[] previousLineAntiAliasedFlags;
    delete[] currentLineAntiAliasedFlags;
}

RenderingConfiguration &
RenderEngine::getMutableConfig()
{
    return const_cast<RenderingConfiguration &>(context->getConfig());
}

inline unsigned short
RenderEngine::rand3dInline(int a, int b)
{
    ProceduralNoise &noise = this->getTextureUtils().getProceduralNoise();
    return noise.checksumTable().eval((int)(noise.hashTable()[(int)(noise.hashTable()[(int)(a & 0xfff)] ^ b) &
                                  0xfff]));
}

ColorRgba *
RenderEngine::allocateColorBuffer(int count)
{
    ColorRgba *buffer = static_cast<ColorRgba *>(
        ::operator new[](sizeof(ColorRgba) * count));
    for (int i = 0; i < count; i++) {
        new (&buffer[i]) ColorRgba(0.0, 0.0, 0.0, 0.0);
    }
    return buffer;
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
        intersection, color, nullptr, true, &engine->traceService,
        &engine->context->getTextureUtils(), *engine->context, engine->traceLevel);
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
        (((double)(this->getScene().getScreenHeight() - 1) - y) - (double)height / 2.0) /
        (double)height;

    const Camera &viewPoint = this->getScene().getViewPoint();
    tempVect1 = viewPoint.getUp().multiply(yScalar);
    tempVect2 = viewPoint.getRight().multiply(xScalar);
    ray->setDirection(tempVect1.add(tempVect2));
    ray->setDirection(ray->getDirection().add(viewPoint.getDirection()));
    ray->setDirection(ray->getDirection().normalizedFast());
    ray->initializeContainers();
    ray->setPrimaryRay(true);
    ray->setQuadricConstantsCached(false);
    if (context) {
        ray->setStatistics(&context->getStatistics());
        ray->setConfig(&context->getConfig());
        ray->setIntersectionQueuePool(&intersectionQueuePool);
    }
}

void
RenderEngine::superSample(
    ColorRgba *result, int x, int y, int width, int height)
{
    ColorRgba color(0.0, 0.0, 0.0, 0.0);
    static const double superSampleOffsets[SUPER_SAMPLE_COUNT][2] = {
        {0.0, 0.0},
        {-SUPER_SAMPLE_CELL_SIZE, -SUPER_SAMPLE_CELL_SIZE},
        {-SUPER_SAMPLE_CELL_SIZE, 0.0},
        {-SUPER_SAMPLE_CELL_SIZE, SUPER_SAMPLE_CELL_SIZE},
        {0.0, -SUPER_SAMPLE_CELL_SIZE},
        {0.0, SUPER_SAMPLE_CELL_SIZE},
        {SUPER_SAMPLE_CELL_SIZE, -SUPER_SAMPLE_CELL_SIZE},
        {SUPER_SAMPLE_CELL_SIZE, 0.0},
        {SUPER_SAMPLE_CELL_SIZE, SUPER_SAMPLE_CELL_SIZE}
    };
    const double dx = (double)x;
    const double dy = (double)y;
    int jitterSeedOffset = JITTER_SEED_INITIAL_OFFSET;

    this->getStatistics().incrementNumberOfPixelsSupersampled();

    result->setR(0.0); result->setG(0.0); result->setB(0.0); result->setA(0);

    for (int sampleIndex = 0; sampleIndex < SUPER_SAMPLE_COUNT; sampleIndex++) {
        const int jitterSeedY = sampleIndex == 0 ? y : y + jitterSeedOffset;
        const double jitterX =
            (this->rand3dInline(x + jitterSeedOffset, jitterSeedY) & JITTER_RANDOM_MASK) /
                JITTER_RANDOM_NORMALIZER * SUPER_SAMPLE_JITTER_RANGE -
            SUPER_SAMPLE_JITTER_BIAS;
        const double jitterY =
            (this->rand3dInline(x + jitterSeedOffset, jitterSeedY) & JITTER_RANDOM_MASK) /
                JITTER_RANDOM_NORMALIZER * SUPER_SAMPLE_JITTER_RANGE -
            SUPER_SAMPLE_JITTER_BIAS;

        this->createRay(this->primaryRay, width, height,
            dx + jitterX + superSampleOffsets[sampleIndex][0],
            dy + jitterY + superSampleOffsets[sampleIndex][1]);
        this->traceLevel = 0;
        this->trace(this->primaryRay, &color);
        ColorOperations::clipColor(&color, &color);
        ColorOperations::scaleColor(&color, &color, SUPER_SAMPLE_WEIGHT);
        ColorOperations::addColor(result, result, &color);

        jitterSeedOffset += JITTER_SEED_INCREMENT;
    }

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
                previousLine, &lineNumber)) == 1) {
    }

    this->getMutableConfig().setFirstLine(lineNumber + 1);

    if (rc == 0) {
        this->getConfig().getOutputFileInputStream()->close();
        if (this->getConfig().getOutputFileInputStream()->open(
                this->getMutableConfig().getOutputFileNameBuffer(),
                &this->getScene().getScreenWidth(),
                &this->getScene().getScreenHeight(),
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
        SceneDumper::dumpSceneStructure(stderr, this->getScene());
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

        for (x = 0; x < this->getScene().getScreenWidth(); x++) {

            if (this->getStopFlag()) {
                if (this->getConfig().getOutputFileInputStream() != nullptr) {
                    this->getConfig().getOutputFileInputStream()->close();
                }
                // Exit with error if image not completed/user abort
                exit(2);
            }

            this->getStatistics().incrementNumberOfPixels();

            this->createRay(this->primaryRay,
                this->getScene().getScreenWidth(),
                this->getScene().getScreenHeight(), (double)x, (double)y);
            this->traceLevel = 0;
            this->trace(&this->ray, &color);
            ColorOperations::clipColor(&color, &color);

            currentLine[x] = color;

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

    if (this->getConfig().hasOptionFlags(RenderingConfiguration::DISK_WRITE)) {
        this->getConfig().getOutputFileInputStream()->writeLine(
            previousLine, this->getConfig().getLastLine() - 1);
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
                this->getScene().getScreenHeight())) {
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
                this->getScene().getScreenWidth(),
                this->getScene().getScreenHeight(),
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
            this->getScene().getScreenWidth(),
            this->getScene().getScreenHeight(),
            (y - this->getConfig().getFirstLine()) + 1,
            this->getConfig().getLastLine() - this->getConfig().getFirstLine());
        if (!this->getConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS)) {
            fprintf(stderr, ".");
        }
    }

    if (this->getConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS)) {
        superSampleCount = 0;
    }
}

void
RenderEngine::doAntiAliasing(int x, int y, ColorRgba *color)
{
    char antialiasCenterFlag = 0;

    currentLineAntiAliasedFlags[x] = 0;

    if (x != 0) {
        if (ColorOperations::colorDistance(&currentLine[x - 1], &currentLine[x]) >=
            this->getScene().getAntialiasThreshold()) {
            antialiasCenterFlag = 1;
            if (!(currentLineAntiAliasedFlags[x - 1])) {
                this->superSample(&currentLine[x - 1], x - 1, y,
                    this->getScene().getScreenWidth(),
                    this->getScene().getScreenHeight());
                currentLineAntiAliasedFlags[x - 1] = 1;
                superSampleCount++;
            }
        }
    }

    if (y != this->getConfig().getFirstLine() - 1) {
        if (ColorOperations::colorDistance(&previousLine[x], &currentLine[x]) >=
            this->getScene().getAntialiasThreshold()) {
            antialiasCenterFlag = 1;
            if (!(previousLineAntiAliasedFlags[x])) {
                this->superSample(&previousLine[x], x, y - 1,
                    this->getScene().getScreenWidth(),
                    this->getScene().getScreenHeight());
                previousLineAntiAliasedFlags[x] = 1;
                superSampleCount++;
            }
        }
    }

    if (antialiasCenterFlag) {
        this->superSample(&currentLine[x], x, y,
            this->getScene().getScreenWidth(),
            this->getScene().getScreenHeight());
        currentLineAntiAliasedFlags[x] = 1;
        *color = currentLine[x];
        superSampleCount++;
    }
}

void
RenderEngine::initializeRenderer()
{
    int i;

    this->primaryRay = &this->ray;
    previousLine = RenderEngine::allocateColorBuffer(this->getScene().getScreenWidth() + 1);
    currentLine = RenderEngine::allocateColorBuffer(this->getScene().getScreenWidth() + 1);

    for (i = 0; i <= this->getScene().getScreenWidth(); i++) {
        previousLine[i].setR(0.0);
        previousLine[i].setG(0.0);
        previousLine[i].setB(0.0);

        currentLine[i].setR(0.0);
        currentLine[i].setG(0.0);
        currentLine[i].setB(0.0);
    }

    if (this->getConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS)) {
        previousLineAntiAliasedFlags =
            new char[(this->getScene().getScreenWidth() + 1)];
        currentLineAntiAliasedFlags =
            new char[(this->getScene().getScreenWidth() + 1)];

        for (i = 0; i <= this->getScene().getScreenWidth(); i++) {
            (previousLineAntiAliasedFlags)[i] = 0;
            (currentLineAntiAliasedFlags)[i] = 0;
        }
    }

    this->ray.setOrigin(this->getScene().getViewPoint().getLocation());
}

void
RenderEngine::outputLine(int y)
{
    ColorRgba *tempColorPtr;
    char *tempCharPtr;

    if (this->getConfig().hasOptionFlags(RenderingConfiguration::DISK_WRITE)) {
        if (y > this->getConfig().getFirstLine()) {
            this->getConfig().getOutputFileInputStream()->writeLine(previousLine, y - 1);
        }
    }

    if (this->getConfig().hasOptionFlags(RenderingConfiguration::VERBOSE)) {
        if (this->getConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS) &&
            this->getConfig().getVerboseFormat() != '1') {
            {
                char _logMsg[1024];
                snprintf(_logMsg, sizeof(_logMsg), " supersampled %d times.", superSampleCount);
                Logger::reportMessage("RenderEngine", Logger::WARNING, "", _logMsg);
            }
        }

        if (this->getConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS) &&
            this->getConfig().getVerboseFormat() == '1') {
            fprintf(stderr, " supersampled %d times.", superSampleCount);
        }
        if (this->getConfig().getVerboseFormat() == '1') {
            fprintf(stderr, "\r");
        } else {
            fprintf(stderr, "\n");
        }
    }
    tempColorPtr = previousLine;
    previousLine = currentLine;
    currentLine = tempColorPtr;

    tempCharPtr = previousLineAntiAliasedFlags;
    previousLineAntiAliasedFlags = currentLineAntiAliasedFlags;
    currentLineAntiAliasedFlags = tempCharPtr;
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

    if (this->traceLevel > (int)this->getMaxTraceLevel()) {
        return;
    }

    if (this->getScene().getFogDistance() == 0.0) {
        color->setR(0.0); color->setG(0.0); color->setB(0.0); color->setA(0);
    } else {
        *color = this->getScene().getFogColor();
    }

    // What objects does this ray intersect?
    const java::ArrayList<BoundedGeometry*> &sceneObjects =
        this->getScene().getObjects();
    for (long int i = sceneObjects.size() - 1; i >= 0; i--) {
        object = sceneObjects[i];
        if (object->intersect(ray, newIntersection)) {
            if (!intersectionFound || newIntersection.getT() < localIntersection.getT()) {
                localIntersection = newIntersection;
            }
            intersectionFound = true;
        }
    }

    if (intersectionFound) {
        RayShaderPipeline::shadeSurface(
            &localIntersection, color, ray, false, this->getTraceService(),
            &this->getTextureUtils(), *context, traceLevel);
    }
}

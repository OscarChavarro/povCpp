#include <cstdio>

#include "java/io/FileOutputStream.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "common/statistics/Statistics.h"
#include "common/RenderRuntimeState.h"
#include "environment/material/RenderOutput.h"
#include "environment/material/RendererConfiguration.h"
#include "render/ColorOperations.h"
#include "render/RayShaderPipeline.h"
#include "render/RenderEngine.h"
#include "render/shaders/ExponentialFogShader.h"
#include "render/shaders/TraceService.h"

RenderEngine::~RenderEngine()
{
}

RenderingConfiguration &
RenderEngine::getMutableConfig()
{
    return const_cast<RenderingConfiguration &>(context->getConfig());
}

void
RenderEngine::createRay(
    RayWithSegments *localRay, int width, int height, double x, double y)
{
    double xScalar;
    double yScalar;
    Vector3Dd temporaryVector1;
    Vector3Dd temporaryVector2;

    // Convert the X Coordinate to be a double from 0.0 to 1.0
    xScalar = (x - (double)width / 2.0) / (double)width;

    // Convert the Y Coordinate to be a double from 0.0 to 1.0
    yScalar =
        (((double)(this->getScene().getScreenHeight() - 1) - y) - (double)height / 2.0) /
        (double)height;

    const Camera &viewPoint = this->getScene().getViewPoint();
    temporaryVector1 = viewPoint.getUp().multiply(yScalar);
    temporaryVector2 = viewPoint.getRight().multiply(xScalar);
    localRay->setDirection(temporaryVector1.add(temporaryVector2));
    localRay->setDirection(
        localRay->getDirection().add(viewPoint.getDirection()));
    localRay->setDirection(localRay->getDirection().normalizedFast());
    localRay->initializeContainers();
    localRay->setPrimaryRay(true);
    localRay->setQuadricConstantsCached(false);
    if (context) {
        localRay->setStatistics(&context->getStatistics());
        localRay->setConfig(&context->getConfig());
        localRay->setIntersectionQueuePool(&intersectionQueuePool);
    }
    localRay->setOrigin(this->getScene().getViewPoint().getLocation());
}

void
RenderEngine::readRenderedPart()
{
    int rc;
    int lineNumber;
    while ((rc = this->getConfig().getOutputFileInputStream()->readLine(
                worker.getPreviousLine(), &lineNumber)) == 1) {
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
    ColorRgba color(0.0, 0.0, 0.0, 0.0);
    RenderWorker &localWorker = this->getWorker();
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
                // Image not completed / user abort. Previously this terminated
                // the whole process with exit(2); under a multi-thread driver
                // that is hostile, so instead report once and fall back to a
                // default (black) colour for the remaining pixels.
                if (!this->fatalErrorFound) {
                    this->fatalErrorFound = true;
                    Logger::reportMessage("RenderEngine", Logger::ERROR,
                        "startTracing",
                        "Rendering aborted before completion; "
                        "remaining pixels filled with default colour\n");
                }
                color.setR(0.0); color.setG(0.0); color.setB(0.0); color.setA(0);
                localWorker.getCurrentLine()[x] = color;
                continue;
            }

            this->getStatistics().incrementNumberOfPixels();

            this->createRay(localWorker.getPrimaryRay(),
                this->getScene().getScreenWidth(),
                this->getScene().getScreenHeight(), (double)x, (double)y);
            localWorker.setTraceLevel(0);
            this->trace(localWorker, &localWorker.getRay(), &color);
            ColorOperations::clipColor(&color, &color);

            localWorker.getCurrentLine()[x] = color;

            if (this->getConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS)) {
                adaptiveAntiAliasing.doAntiAliasing(localWorker, x, y, &color);
            }

            if (y != this->getConfig().getFirstLine() - 1) {
                if (this->getConfig().hasOptionFlags(RenderingConfiguration::DISPLAY)) {
                    (void)x;
                    (void)y;
                }
            }
        }
        this->outputLine(localWorker, y);
    }

    if (this->getConfig().hasOptionFlags(RenderingConfiguration::DISK_WRITE)) {
        this->getConfig().getOutputFileInputStream()->writeLine(
            localWorker.getPreviousLine(), this->getConfig().getLastLine() - 1);
    }
}

void
RenderEngine::checkStats(int y)
{
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
RenderEngine::initializeRenderer()
{
    worker.initializeLineBuffers(
        this->getScene().getScreenWidth(),
        this->getConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS));
    worker.getRay().setOrigin(this->getScene().getViewPoint().getLocation());
}

void
RenderEngine::outputLine(RenderWorker &localWorker, int y)
{
    if (this->getConfig().hasOptionFlags(RenderingConfiguration::DISK_WRITE)) {
        if (y > this->getConfig().getFirstLine()) {
            this->getConfig().getOutputFileInputStream()->writeLine(localWorker.getPreviousLine(), y - 1);
        }
    }

    if (this->getConfig().hasOptionFlags(RenderingConfiguration::VERBOSE)) {
        if (this->getConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS) &&
            this->getConfig().getVerboseFormat() != '1') {
            {
                char _logMsg[1024];
                snprintf(_logMsg, sizeof(_logMsg), " super-sampled %d times.", superSampleCount);
                Logger::reportMessage("RenderEngine", Logger::WARNING, "", _logMsg);
            }
        }

        if (this->getConfig().hasOptionFlags(RenderingConfiguration::ANTIALIAS) &&
            this->getConfig().getVerboseFormat() == '1') {
            fprintf(stderr, " super-sampled %d times.", superSampleCount);
        }
        if (this->getConfig().getVerboseFormat() == '1') {
            fprintf(stderr, "\r");
        } else {
            fprintf(stderr, "\n");
        }
    }
    localWorker.swapLines();
}

void
RenderEngine::trace(RenderWorker &localWorker, RayWithSegments *localRay, ColorRgba *color)
{
    BoundedGeometry *object;
    Intersection localIntersection;
    Intersection newIntersection;
    bool intersectionFound;

    this->getStatistics().incrementNumberOfRays();
    color->setR(0.0); color->setG(0.0); color->setB(0.0); color->setA(0);

    intersectionFound = false;

    if (localWorker.getTraceLevel() > (int)this->getMaxTraceLevel()) {
        return;
    }

    if (this->getScene().getFogDistance() == 0.0) {
        color->setR(0.0); color->setG(0.0); color->setB(0.0); color->setA(0);
    } else {
        *color = this->getScene().getFogColor();
    }

    const java::ArrayList<BoundedGeometry*> &sceneObjects =
        this->getScene().getObjects();
    for (long int i = sceneObjects.size() - 1; i >= 0; i--) {
        object = sceneObjects[i];
        if (object->intersect(localRay, newIntersection)) {
            if (!intersectionFound || newIntersection.getT() < localIntersection.getT()) {
                localIntersection = newIntersection;
            }
            intersectionFound = true;
        }
    }

    if (intersectionFound) {
        RayShaderPipeline::shadeSurface(
            &localIntersection, color, localRay, false,
            localWorker.getTraceService(), &this->getTextureUtils(), *context,
            localWorker.getTraceLevel());
    }
}

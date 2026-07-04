#include "java/io/FileOutputStream.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"
#include "java/util/concurrent/Executors.h"
#include "java/util/concurrent/Void.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.txx"
#include "vsdk/toolkit/render/raytracing/RasterTileGenerator.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/PovRayHit.h"
#include "environment/geometry/element/PriorityQueuePool.txx"
#include "environment/material/povray/PovRayMaterial.h"
#include "environment/material/RenderOutput.h"
#include "environment/material/PovRayRendererConfiguration.h"
#include "render/shaders/TraceService.h"
#include "render/ColorOperations.h"
#include "render/bakedScene/BakedSceneBuilder.h"
#include "render/bakedScene/BakedTrace.h"
#include "render/RayShaderPipeline.h"
#include "render/RenderEngine.h"
#include "render/RenderTask.h"
#include "render/RenderTileCallable.h"

bool
RenderEngine::rayIntersectsAabbBefore(
    const RayWithSegments &ray, const AxisAlignedBoundingBox &box, double maxT)
{
    const Vector3Dd origin = ray.getOrigin();
    const Vector3Dd direction = ray.getDirection();
    double tMin = 0.0;
    double tMax = maxT;

    auto updateAxis = [&](double originCoord, double directionCoord,
                          double minCoord, double maxCoord) -> bool {
        if (directionCoord > -1e-12 && directionCoord < 1e-12) {
            return originCoord >= minCoord && originCoord <= maxCoord;
        }
        const double invDir = 1.0 / directionCoord;
        double nearT = (minCoord - originCoord) * invDir;
        double farT = (maxCoord - originCoord) * invDir;
        if (nearT > farT) {
            const double tmp = nearT;
            nearT = farT;
            farT = tmp;
        }
        tMin = nearT > tMin ? nearT : tMin;
        tMax = farT < tMax ? farT : tMax;
        return tMin <= tMax;
    };

    return
        updateAxis(origin.x(), direction.x(), box.min.x(), box.max.x()) &&
        updateAxis(origin.y(), direction.y(), box.min.y(), box.max.y()) &&
        updateAxis(origin.z(), direction.z(), box.min.z(), box.max.z()) &&
        tMax >= 0.0;
}

java::Void
RenderTileCallable::call()
{
    engine->renderTile(
        task->worker, task->pool, task->statistics, task->area);
    return java::Void();
}

RenderEngine::RenderEngine()
    : context(nullptr), scene(nullptr), maxTraceLevelValue(nullptr),
      stopFlagValue(nullptr), worker(this),
      adaptiveAntiAliasing(this),
      imageWriter(this),
      fatalErrorFound(false)
{
}

RenderEngine::~RenderEngine()
{
}

void
RenderEngine::buildBakedScene()
{
    BakedSceneBuilder::build(scene->getObjects(), bakedScene);
    context->setBakedScene(bakedScene);
}

PovRayRendererConfiguration &
RenderEngine::getMutableConfig()
{
    return const_cast<PovRayRendererConfiguration &>(context->getConfig());
}

void
RenderEngine::createRay(
    RayWithSegments *localRay, int width, int height, double x, double y,
    PriorityQueuePool<IntersectionCandidate> *pool, Statistics *stats)
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

    const CameraSnapshot &viewPoint = this->getScene().getViewPoint();
    temporaryVector1 = viewPoint.getUpWithScale().multiply(yScalar);
    temporaryVector2 = viewPoint.getRightWithScale().multiply(xScalar);
    localRay->setDirection(temporaryVector1.add(temporaryVector2));
    localRay->setDirection(
        localRay->getDirection().add(viewPoint.getDir()));
    localRay->setDirection(localRay->getDirection().normalizedFast());
    localRay->initializeContainers();
    localRay->setPrimaryRay(true);
    localRay->setQuadricConstantsCached(false);
    if (context) {
        // The whole recursive ray tree (reflection/refraction/shadow rays)
        // inherits this pointer via setStatistics(parent->getStatistics()),
        // so routing the primary ray to the calling task's own Statistics is
        // enough to make every per-primitive intersection counter (B5)
        // thread-safe without touching any shader/geometry call site.
        localRay->setStatistics(stats);
        localRay->setConfig(&context->getConfig());
        localRay->setIntersectionQueuePool(pool);
    }
    localRay->setOrigin(this->getScene().getViewPoint().getEyePosition());
}

void
RenderEngine::readRenderedPart()
{
    imageWriter.readRenderedPart(worker.getPreviousLine());
}

void
RenderEngine::copyLineToImage(
    const ColorRgba *line, int row, const RasterTileArea &area)
{
    for (int x = area.getX0(); x < area.getX1(); x++) {
        destinationImage.setPixel(x, row, line[x]);
    }
}

void
RenderEngine::persistDestinationImage()
{
    const PovRayRendererConfiguration &config = this->getConfig();
    if (!config.hasOptionFlags(PovRayRendererConfiguration::DISK_WRITE)) {
        return;
    }
    RenderOutput *out = config.getOutputFileInputStream();
    for (int y = config.getFirstLine(); y < config.getLastLine(); y++) {
        out->writeLine(destinationImage.rowPointer(y), y);
    }
}

void
RenderEngine::renderTile(
    RenderWorker &localWorker, PriorityQueuePool<IntersectionCandidate> &pool,
    Statistics &stats, const RasterTileArea &area)
{
    ColorRgba color(0.0, 0.0, 0.0, 0.0);
    int x;
    int y;
    for (y = this->getConfig().hasOptionFlags(PovRayRendererConfiguration::ANTIALIAS)
                ? area.getY0() - 1
                : area.getY0();
        y < area.getY1();
        y++) {

        this->checkStats(y);

        for (x = area.getX0(); x < area.getX1(); x++) {

            if (this->getStopFlag()) {
                // Image not completed / user abort. Previously this terminated
                // the whole process with exit(2); under a multi-thread driver
                // that is hostile, so instead report once (atomically, so
                // concurrent RenderTask threads never double-report) and fall
                // back to a default (black) colour for the remaining pixels.
                bool alreadyReported = false;
                if (this->fatalErrorFound.compare_exchange_strong(
                        alreadyReported, true)) {
                    Logger::reportMessage("RenderEngine", Logger::ERROR,
                        "startTracing",
                        "Rendering aborted before completion; "
                        "remaining pixels filled with default colour\n");
                }
                color.setR(0.0); color.setG(0.0); color.setB(0.0); color.setA(0);
                localWorker.getCurrentLine()[x] = color;
                continue;
            }

            stats.incrementNumberOfPixels();

            this->createRay(localWorker.getPrimaryRay(),
                this->getScene().getScreenWidth(),
                this->getScene().getScreenHeight(), (double)x, (double)y,
                &pool, &stats);
            localWorker.setTraceLevel(0);
            this->trace(localWorker, &localWorker.getRay(), &color);
            ColorOperations::clipColor(&color, &color);

            localWorker.getCurrentLine()[x] = color;

            if (this->getConfig().hasOptionFlags(PovRayRendererConfiguration::ANTIALIAS)) {
                adaptiveAntiAliasing.doAntiAliasing(
                    localWorker, x, y, &color, area, &pool, &stats);
            }

            if (y != area.getY0() - 1) {
                if (this->getConfig().hasOptionFlags(PovRayRendererConfiguration::DISPLAY)) {
                    (void)x;
                    (void)y;
                }
            }
        }
        // previousLine (row y-1) is final, post-AA, exactly when the streaming
        // writer used to flush it (y > area.getY0()); copy it into the image
        // instead of writing it straight to disk, then recycle the buffers.
        if (y > area.getY0()) {
            copyLineToImage(localWorker.getPreviousLine(), y - 1, area);
        }
        localWorker.swapLines();
    }

    copyLineToImage(localWorker.getPreviousLine(), area.getY1() - 1, area);
}

void
RenderEngine::startTracing()
{
    renderTile(worker, intersectionQueuePool, context->getStatistics(), renderArea);
    persistDestinationImage();
}

void
RenderEngine::startTracingParallel()
{
    const int threads = this->getConfig().getNumberOfThreads();

    RasterTileGenerator generator(RasterTileGenerationStrategy::LINEAR,
        &destinationImage, renderArea.getX0(), renderArea.getY0(),
        renderArea.getDx(), renderArea.getDy(), threads);
    const java::ArrayList<RasterTileArea> &tileAreas = generator.getTiles();

    const int w = this->getScene().getScreenWidth();
    const bool antialiasEnabled =
        this->getConfig().hasOptionFlags(PovRayRendererConfiguration::ANTIALIAS);

    java::ArrayList<RenderTask *> tasks;
    tasks.reserve(tileAreas.size());
    for (long i = 0; i < tileAreas.size(); i++) {
        RenderTask *task = new RenderTask(this, tileAreas[i]);
        // Each task's RenderWorker needs its own line buffers (this is only
        // ever done once, engine-wide, for the serial worker member, in
        // initializeRenderer); ray.setOrigin mirrors that same setup.
        task->worker.initializeLineBuffers(w, antialiasEnabled);
        task->worker.getRay().setOrigin(
            this->getScene().getViewPoint().getEyePosition());

        // B6: bind a TextureUtils private to this task, so noise()/
        // differentialNoise() call counters land in task->statistics's own
        // SolidTextureStatistics instead of racing on a shared one. This MUST
        // run here, serially, one task at a time, before any thread starts:
        // ProceduralNoise::initialize() reseeds the C `rand()` stream with
        // srand(0) and consumes it deterministically (permutation shuffle,
        // then the wave table), so calling it serially N times reproduces
        // the exact same tables N times — bit-identical to each other and to
        // the single serial-path instance. Calling it concurrently would
        // corrupt that shared, process-wide rand() stream instead.
        task->textureUtils.initialize(task->statistics.getSolidTextureStatistics());
        task->textureUtils.initializeNoise(PovRayMaterial::DEFAULT_NUMBER_OF_WAVES);
        task->worker.setTextureUtils(&task->textureUtils);

        tasks.add(task);
    }

    java::ExecutorService *threadPool =
        java::Executors::newFixedThreadPool((int)tasks.size());

    {
        char _logMsg[256];
        snprintf(_logMsg, sizeof(_logMsg),
            "Created %d render thread(s) for %ld tile(s)\n",
            (int)tasks.size(), tasks.size());
        Logger::reportMessage("RenderEngine", Logger::WARNING,
            "startTracingParallel", _logMsg);
    }

    java::ArrayList<java::Future<java::Void>> futures;
    futures.reserve(tasks.size());
    for (long i = 0; i < tasks.size(); i++) {
        futures.add(
            threadPool->submit(new RenderTileCallable(this, tasks[i])));
    }
    // get() blocks until each tile's task has finished (join) and rethrows
    // any exception the worker thread caught while rendering its tile.
    for (long i = 0; i < futures.size(); i++) {
        futures[i].get();
    }
    threadPool->shutdownNow();
    delete threadPool;

    // B5: reduce every task's own Statistics into the shared total using the
    // existing parts-summing constructor, exactly as a single-task reduction
    // would (so serial-equivalent numbers come out the other end).
    java::ArrayList<Statistics *> statisticsParts;
    statisticsParts.reserve(tasks.size());
    for (long i = 0; i < tasks.size(); i++) {
        statisticsParts.add(&tasks[i]->statistics);
    }
    context->getStatistics() = Statistics(&statisticsParts);

    for (long i = 0; i < tasks.size(); i++) {
        delete tasks[i];
    }

    persistDestinationImage();
}

void
RenderEngine::checkStats(int y)
{
    if (this->getConfig().hasOptionFlags(PovRayRendererConfiguration::VERBOSE) &&
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
        if (!this->getConfig().hasOptionFlags(PovRayRendererConfiguration::ANTIALIAS)) {
            Logger::reportMessage("RenderEngine", Logger::WARNING, "", ".");
        }
    }
    if (this->getConfig().hasOptionFlags(PovRayRendererConfiguration::VERBOSE_FILE)) {
        java::FileOutputStream statFile(this->getConfig().getStatFileName());
        char buf[32];
        snprintf(buf, sizeof(buf), "Line %4d.\n", y);
        for (int i = 0; buf[i] != '\0'; i++) {
            statFile.write((unsigned char)buf[i]);
        }
        statFile.close();
    }

    // Use -vO for Old style verbose
    if (this->getConfig().hasOptionFlags(PovRayRendererConfiguration::VERBOSE) &&
        (this->getConfig().getVerboseFormat() == 'O')) {
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "Line %4d", y);
            Logger::reportMessage("RenderEngine", Logger::WARNING, "", _logMsg);
        }
    }
    if (this->getConfig().hasOptionFlags(PovRayRendererConfiguration::VERBOSE) &&
        this->getConfig().getVerboseFormat() == '1') {
        fprintf(stderr, "Res %4d X %4d. Calc line %4d of %4d",
            this->getScene().getScreenWidth(),
            this->getScene().getScreenHeight(),
            (y - this->getConfig().getFirstLine()) + 1,
            this->getConfig().getLastLine() - this->getConfig().getFirstLine());
        if (!this->getConfig().hasOptionFlags(PovRayRendererConfiguration::ANTIALIAS)) {
            fprintf(stderr, ".");
        }
    }
}

void
RenderEngine::initializeRenderer()
{
    const int w = this->getScene().getScreenWidth();
    const int h = this->getScene().getScreenHeight();
    destinationImage.allocate(w, h);
    // The engine's own render area is the [firstLine, lastLine) window it is
    // responsible for tracing (the full image unless -s/-e narrow it), not
    // necessarily the whole destination image. This is what the M1 AA clip
    // in AdaptiveAntiAliasing compares against.
    renderArea = RasterTileArea(&destinationImage, 0,
        this->getConfig().getFirstLine(), w,
        this->getConfig().getLastLine() - this->getConfig().getFirstLine());
    worker.initializeLineBuffers(
        w, this->getConfig().hasOptionFlags(PovRayRendererConfiguration::ANTIALIAS));
    worker.getRay().setOrigin(this->getScene().getViewPoint().getEyePosition());
    // Serial mode: the engine's own worker shares the engine-wide TextureUtils
    // (already initialized by PovRayApplication::prepareRendering), exactly
    // like before this was made an explicit per-worker pointer.
    worker.setTextureUtils(&context->getTextureUtils());
}

void
RenderEngine::trace(RenderWorker &localWorker, RayWithSegments *localRay, ColorRgba *color)
{
    IntersectionCandidate localIntersection;
    IntersectionCandidate newIntersection;
    bool intersectionFound;

    localRay->getStatistics()->incrementNumberOfRays();
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

    const BakedScene &bakedScene = this->bakedScene;
    RaySharedCache &raySharedCache = localWorker.getRaySharedCache();
    raySharedCache.ensureCapacity(
        (int)bakedScene.statistics.quadricViewpointSlotCount,
        (int)bakedScene.statistics.planeViewpointSlotCount);
    const java::ArrayList<int> &boundedObjects = bakedScene.boundedObjectIndices;
    const java::ArrayList<int> &unboundedObjects = bakedScene.unboundedObjectIndices;
    for (long int i = boundedObjects.size() - 1; i >= 0; i--) {
        const int objectIndex = boundedObjects[i];
        const double currentBestT =
            (intersectionFound && localIntersection.getIntersection().t > 0.0) ?
                localIntersection.getIntersection().t : 1e30;
        if (!rayIntersectsAabbBefore(
                *localRay, bakedScene.traceableObjects[objectIndex].worldBounds, currentBestT)) {
            continue;
        }
        const bool hit = BakedTrace::traceFirstHit(
            bakedScene, objectIndex, localRay, newIntersection, raySharedCache);
        if (hit) {
            if (!intersectionFound ||
                newIntersection.getIntersection().t <
                    localIntersection.getIntersection().t) {
                localIntersection = newIntersection;
            }
            intersectionFound = true;
        }
    }
    for (long int i = unboundedObjects.size() - 1; i >= 0; i--) {
        const int objectIndex = unboundedObjects[i];
        const bool hit = BakedTrace::traceFirstHit(
            bakedScene, objectIndex, localRay, newIntersection, raySharedCache);
        if (hit) {
            if (!intersectionFound ||
                newIntersection.getIntersection().t <
                    localIntersection.getIntersection().t) {
                localIntersection = newIntersection;
            }
            intersectionFound = true;
        }
    }

    if (intersectionFound) {
        // Compute the surface normal once, here, for the single winning hit
        // only - never per-candidate, never for shadow rays at this call site -
        // then carry it by value inside Intersection so both shading read
        // sites (LocalSurfaceShader, RayShaderPipeline's refraction branch)
        // reuse it instead of each re-deriving it from the owner body.
        //
        // Only those two read sites ever look at the normal, and both are
        // gated behind withSurfaceLighting()/withRefraction() (see
        // doc/vitralNormalizationAnalysis.md §7.2); withRefraction() can only be
        // true when withSurfaceLighting() already is, so a single flag check
        // here is exactly the per-ray "needs normal" decision.
        localRay->setRequiredDetailMask(
            localRay->getConfig()->withSurfaceLighting()
                ? RayWithSegments::DETAIL_ALL
                : RayWithSegments::DETAIL_NONE);

        if (localRay->needsNormal()) {
            PovRayHit winningHit;
            winningHit.p = localIntersection.getIntersection().point;
            winningHit.n = localIntersection.getIntersection().normal;
            RayOperationOwner *hitBody = localIntersection.getAttributes().getHitBody();
            Geometry *hitGeometry = localIntersection.getAttributes().getHitGeometry();
            winningHit.hitGeometry = hitGeometry;
            winningHit.detailOwnerCount =
                localIntersection.getAttributes().getDetailOwnerCount();
            for (int i = 0; i < winningHit.detailOwnerCount; i++) {
                winningHit.detailOwners[i] =
                    localIntersection.getAttributes().getDetailOwnerAt(i);
            }
            if (hitBody != nullptr) {
                hitBody->doExtraInformation(
                    *localRay, localIntersection.getIntersection().t, &winningHit);
            } else if (hitGeometry != nullptr) {
                hitGeometry->doExtraInformation(
                    *localRay, localIntersection.getIntersection().t, &winningHit);
            }
            localIntersection.getIntersection().normal = winningHit.n;
        }

        // localWorker's own TextureUtils (the shared engine instance in
        // serial mode, or this task's private instance in parallel mode) —
        // never the engine's, so noise() call counters never race (B6).
        RayShaderPipeline::shadeSurface(
            &localIntersection, color, localRay, false,
            localWorker.getTraceService(), &localWorker.getTextureUtils(),
            *context, localWorker.getTraceLevel());
    }
}

#include "java/lang/Exception.h"
#include "java/lang/String.h"
#include <cstdio>

#include "java/util/ArrayList.txx"
#include "java/util/concurrent/Callable.h"
#include "java/util/concurrent/ConcurrentLinkedQueue.h"
#include "java/util/concurrent/ExecutorService.h"
#include "java/util/concurrent/Executors.h"
#include "java/util/concurrent/Future.h"
#include "java/util/concurrent/Void.h"
#include <pthread.h>
#include <unistd.h>
#include "vsdk/toolkit/common/VSDKFatalException.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/gui/feedback/ProgressMonitor.h"
#include "vsdk/toolkit/gui/feedback/parallel/ParallelProgressMonitorConsumer.h"
#include "vsdk/toolkit/gui/feedback/parallel/ParallelProgressMonitorEvent.h"
#include "vsdk/toolkit/gui/feedback/parallel/ParallelProgressMonitorProducer.h"
#include "vsdk/toolkit/media/RGBImageUncompressed.h"
#include "vsdk/toolkit/media/ZBuffer.h"
#include "vsdk/toolkit/render/raytracing/DepthBufferEncoder.h"
#include "vsdk/toolkit/environment/material/RendererConfiguration.h"
#include "vsdk/toolkit/environment/scene/SimpleSceneSnapshot.h"
#include "vsdk/toolkit/render/raytracing/ParallelRaytracer.h"
#include "vsdk/toolkit/render/raytracing/RasterTileArea.h"
#include "vsdk/toolkit/render/raytracing/RasterTileGenerationStrategy.h"
#include "vsdk/toolkit/render/raytracing/RasterTileGenerator.h"
#include "vsdk/toolkit/render/raytracing/SimpleRaytracer.h"

struct ParallelRaytracerConsumerThreadData {
    ParallelProgressMonitorConsumer* consumer;
};

class ParallelRaytracerTileWorker : public java::Callable<java::Void> {
  private:
    java::ConcurrentLinkedQueue<RasterTileArea>* pendingTiles;
    RGBImageUncompressed* resultingImage;
    ZBuffer* depthBuffer;
    const DepthBufferEncoder* depthEncoder;
    const RendererConfiguration* rendererConfiguration;
    SimpleSceneSnapshot* sceneSnapshot;
    ProgressMonitor* progressReporter;

  public:
    ParallelRaytracerTileWorker(java::ConcurrentLinkedQueue<RasterTileArea>* pendingTiles,
               RGBImageUncompressed* resultingImage,
               ZBuffer* depthBuffer,
               const DepthBufferEncoder* depthEncoder,
               const RendererConfiguration* rendererConfiguration,
               SimpleSceneSnapshot* sceneSnapshot,
               ProgressMonitor* progressReporter)
        : pendingTiles(pendingTiles),
          resultingImage(resultingImage),
          depthBuffer(depthBuffer),
          depthEncoder(depthEncoder),
          rendererConfiguration(rendererConfiguration),
          sceneSnapshot(sceneSnapshot),
          progressReporter(progressReporter)
    {
    }

    virtual java::Void call() override
    {
        RasterTileArea tile(resultingImage, 0, 0,
            resultingImage->getXSize(), resultingImage->getYSize());
        SimpleRaytracer raytracer;

        while ( pendingTiles->poll(&tile) ) {
            raytracer.execute(resultingImage,
                              rendererConfiguration,
                              sceneSnapshot,
                              progressReporter,
                              depthBuffer,
                              depthEncoder,
                              tile.getStartX(),
                              tile.getStartY(),
                              tile.getEndX(),
                              tile.getEndY());
        }
        return java::Void();
    }
};

ParallelRaytracer::ParallelRaytracer()
    : numberOfThreads(availableProcessors()),
      executorService(0),
      depthBufferMode(DepthBufferMode::NONE),
      openGlDepthRangeNear(0.0),
      openGlDepthRangeFar(1.0),
      depthBuffer(0)
{
}

ParallelRaytracer::ParallelRaytracer(int numberOfThreads)
    : numberOfThreads(numberOfThreads),
      executorService(0),
      depthBufferMode(DepthBufferMode::NONE),
      openGlDepthRangeNear(0.0),
      openGlDepthRangeFar(1.0),
      depthBuffer(0)
{
    if ( numberOfThreads <= 0 ) {
        Logger::reportMessage("ParallelRaytracer", Logger::ERROR, "ParallelRaytracer",
            "numberOfThreads must be > 0");
        throw VSDKFatalException("numberOfThreads must be > 0");
    }
}

ParallelRaytracer::~ParallelRaytracer()
{
    dispose();
    delete depthBuffer;
}

void ParallelRaytracer::setDepthBufferMode(DepthBufferMode mode)
{
    depthBufferMode = mode;
    if ( depthBufferMode == DepthBufferMode::NONE ) {
        delete depthBuffer;
        depthBuffer = 0;
    }
}

DepthBufferMode ParallelRaytracer::getDepthBufferMode() const
{
    return depthBufferMode;
}

void ParallelRaytracer::setOpenGlDepthRange(double nearValue, double farValue)
{
    openGlDepthRangeNear = nearValue;
    openGlDepthRangeFar = farValue;
}

double ParallelRaytracer::getOpenGlDepthRangeNear() const
{
    return openGlDepthRangeNear;
}

double ParallelRaytracer::getOpenGlDepthRangeFar() const
{
    return openGlDepthRangeFar;
}

ZBuffer* ParallelRaytracer::getDepthBuffer() const
{
    return depthBuffer;
}

int ParallelRaytracer::availableProcessors()
{
#ifdef VITRAL_WITH_POSIX_THREADS
    long cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
    return cpuCount > 0 ? (int)cpuCount : 1;
#else
    // Without POSIX threads the pool runs each task in the caller
    return 1;
#endif
}

int ParallelRaytracer::getNumberOfThreads() const
{
    return numberOfThreads;
}

java::ExecutorService* ParallelRaytracer::getExecutorService()
{
    if ( executorService == 0 ) {
        executorService = java::Executors::newFixedThreadPool(numberOfThreads);
    }
    return executorService;
}

void* ParallelRaytracer::progressConsumerMain(void* arg)
{
    ParallelRaytracerConsumerThreadData* data =
        reinterpret_cast<ParallelRaytracerConsumerThreadData*>(arg);
    data->consumer->run();
    return 0;
}

long long ParallelRaytracer::calculateTotalProgressElements(
    const java::ArrayList<RasterTileArea>& generatedTiles)
{
    long long totalElements = 0;

    for ( long int i = 0; i < generatedTiles.size(); i++ ) {
        totalElements += generatedTiles.get(i).getHeight();
    }
    return totalElements;
}

void ParallelRaytracer::execute(
    RGBImageUncompressed* resultingImage,
    const RendererConfiguration* rendererConfiguration,
    SimpleSceneSnapshot* sceneSnapshot,
    bool reportProgress)
{
    if ( resultingImage->getXSize() <= 0 || resultingImage->getYSize() <= 0 ) {
        return;
    }
    if ( depthBufferMode == DepthBufferMode::NONE ) {
        executeWithEncoder(resultingImage, 0, 0, rendererConfiguration,
            sceneSnapshot, reportProgress);
        return;
    }
    if ( depthBuffer == 0 ||
         depthBuffer->getXSize() != resultingImage->getXSize() ||
         depthBuffer->getYSize() != resultingImage->getYSize() ) {
        delete depthBuffer;
        depthBuffer = new ZBuffer(resultingImage->getXSize(),
            resultingImage->getYSize());
    }
    DepthBufferEncoder depthEncoder(depthBufferMode,
        sceneSnapshot->getCameraSnapshot(), openGlDepthRangeNear, openGlDepthRangeFar);
    executeWithEncoder(resultingImage, depthBuffer, &depthEncoder,
        rendererConfiguration, sceneSnapshot, reportProgress);
}

void ParallelRaytracer::execute(
    RGBImageUncompressed* resultingImage,
    ZBuffer* outDepth,
    DepthBufferMode depthMode,
    const RendererConfiguration* rendererConfiguration,
    SimpleSceneSnapshot* sceneSnapshot,
    bool reportProgress)
{
    if ( outDepth == 0 || depthMode == DepthBufferMode::NONE ) {
        executeWithEncoder(resultingImage, 0, 0, rendererConfiguration,
            sceneSnapshot, reportProgress);
        return;
    }
    if ( outDepth->getXSize() != resultingImage->getXSize() ||
         outDepth->getYSize() != resultingImage->getYSize() ) {
        Logger::reportMessage("ParallelRaytracer", Logger::ERROR, "execute",
            "Depth buffer size must match the image size");
        throw VSDKFatalException("Depth buffer size must match the image size");
    }
    DepthBufferEncoder depthEncoder(depthMode,
        sceneSnapshot->getCameraSnapshot(), openGlDepthRangeNear, openGlDepthRangeFar);
    executeWithEncoder(resultingImage, outDepth, &depthEncoder,
        rendererConfiguration, sceneSnapshot, reportProgress);
}

void ParallelRaytracer::executeWithEncoder(
    RGBImageUncompressed* resultingImage,
    ZBuffer* outDepth,
    const DepthBufferEncoder* depthEncoder,
    const RendererConfiguration* rendererConfiguration,
    SimpleSceneSnapshot* sceneSnapshot,
    bool reportProgress)
{
    if ( resultingImage->getXSize() <= 0 || resultingImage->getYSize() <= 0 ) {
        return;
    }
    RasterTileGenerator tileGenerator(
        RasterTileGenerationStrategy::LINEAR,
        resultingImage,
        resultingImage->getXSize(),
        resultingImage->getYSize(),
        numberOfThreads * BANDS_PER_THREAD);
    java::ArrayList<RasterTileArea> generatedTiles = tileGenerator.getTiles();
    java::ConcurrentLinkedQueue<RasterTileArea> pendingTiles(generatedTiles);

    java::ConcurrentLinkedQueue<ParallelProgressMonitorEvent> progressEvents;
    ParallelProgressMonitorProducer producer(&progressEvents);
    ParallelProgressMonitorConsumer consumer(&progressEvents);
    ProgressMonitor* progressReporter = 0;

#ifdef VITRAL_WITH_POSIX_THREADS
    pthread_t consumerThread;
    ParallelRaytracerConsumerThreadData consumerData;
    consumerData.consumer = &consumer;
#endif

    if ( reportProgress ) {
        printf("Starting parallel raytracing with %d threads.\n", numberOfThreads);
        producer.init(calculateTotalProgressElements(generatedTiles));
        progressReporter = &producer;
#ifdef VITRAL_WITH_POSIX_THREADS
        pthread_create(&consumerThread, 0, &progressConsumerMain, &consumerData);
#endif
    }

    java::ArrayList<java::Future<java::Void> > futures;
    for ( int i = 0; i < numberOfThreads; i++ ) {
        futures.add(getExecutorService()->submit(new ParallelRaytracerTileWorker(
            &pendingTiles,
            resultingImage,
            depthEncoder != 0 ? outDepth : 0,
            depthEncoder,
            rendererConfiguration,
            sceneSnapshot,
            progressReporter)));
    }

    bool failed = false;
    java::String failure;
    for ( long int i = 0; i < futures.size(); i++ ) {
        try {
            futures[i].get();
        }
        catch ( const java::Exception& e ) {
            if ( !failed ) {
                failed = true;
                failure = java::String("Parallel raytracing failed: ").concat(e.getMessage().c_str());
            }
        }
    }

    if ( reportProgress ) {
        producer.finish();
#ifdef VITRAL_WITH_POSIX_THREADS
        pthread_join(consumerThread, 0);
#else
        consumer.run();
#endif
    }

    if ( failed ) {
        Logger::reportMessage("ParallelRaytracer", Logger::ERROR, "execute", failure.toCString());
        throw VSDKFatalException(failure);
    }
    if ( !pendingTiles.isEmpty() ) {
        Logger::reportMessage("ParallelRaytracer", Logger::ERROR, "execute",
            "Parallel raytracing finished with pending tiles");
        throw VSDKFatalException("Parallel raytracing finished with pending tiles");
    }
}

void ParallelRaytracer::dispose()
{
    if ( executorService != 0 ) {
        executorService->shutdownNow();
        delete executorService;
        executorService = 0;
    }
}

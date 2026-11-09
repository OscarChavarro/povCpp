#ifndef __PARALLEL_RAYTRACER__
#define __PARALLEL_RAYTRACER__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/render/raytracing/RasterTileArea.h"
#include "vsdk/toolkit/render/raytracing/DepthBufferMode.h"

namespace java {
class ExecutorService;
}
class RGBImageUncompressed;
class RendererConfiguration;
class SimpleSceneSnapshot;
class ZBuffer;
class DepthBufferEncoder;

/**
Raytraces an image with as many threads as processors are available to the
process. The image is split in horizontal bands (see `RasterTileGenerator`),
several per thread, and each thread takes the next pending band when it
finishes one, so threads stay busy even if some areas of the image are much
more expensive than others. Each thread uses its own `SimpleRaytracer` over the
same (read only) scene snapshot, and writes disjoint rows of the image.

The threads are created once and reused by every call, so this class suits
interactive use (one image per frame). `dispose` (and the destructor) stops
them.

Optionally (see `setDepthBufferMode`), each call also exports the depth of
the primary ray of every pixel into a `ZBuffer` of the size of the image:
native ray distances, or depth values normalized as OpenGL would store them
for the same camera (with its near and far planes and the configured
`glDepthRange`), so the raytraced image can be composited with rasterized
geometry (see `DepthBufferMode`). Rows of the depth buffer follow the rows of
the image: row 0 is the top one. Unlike Java, the configuration methods are
not synchronized: call them from the thread that calls `execute`.

C++ counterpart of Java's `vsdk.toolkit.render.raytracing.ParallelRaytracer`.
*/
class ParallelRaytracer {
  private:
    /** Bands per thread: more bands balance the load better */
    static const int BANDS_PER_THREAD = 8;

    int numberOfThreads;
    java::ExecutorService* executorService;

    DepthBufferMode depthBufferMode;
    double openGlDepthRangeNear;
    double openGlDepthRangeFar;
    /// Depth buffer of the last `execute` call, reused while its size fits
    ZBuffer* depthBuffer;

    void executeWithEncoder(
        RGBImageUncompressed* resultingImage,
        ZBuffer* outDepth,
        const DepthBufferEncoder* depthEncoder,
        const RendererConfiguration* rendererConfiguration,
        SimpleSceneSnapshot* sceneSnapshot,
        bool reportProgress);

    java::ExecutorService* getExecutorService();
    static int availableProcessors();
    static long long calculateTotalProgressElements(
        const java::ArrayList<RasterTileArea>& generatedTiles);
    static void* progressConsumerMain(void* arg);

    // Threads are owned: copying would share them
    ParallelRaytracer(const ParallelRaytracer& other);
    ParallelRaytracer& operator=(const ParallelRaytracer& other);

  public:
    /**
    Creates a raytracer with one thread per available processor.
    */
    ParallelRaytracer();

    /**
    @param numberOfThreads number of threads to render with (at least 1)
    */
    explicit ParallelRaytracer(int numberOfThreads);

    ~ParallelRaytracer();

    /**
    @return number of threads used to render
    */
    int getNumberOfThreads() const;

    /**
    Selects whether (and how) the next `execute` calls export a depth buffer.
    @param mode kind of depth buffer
    */
    void setDepthBufferMode(DepthBufferMode mode);

    /**
    @return kind of depth buffer exported by `execute`
    */
    DepthBufferMode getDepthBufferMode() const;

    /**
    Sets the mapping from normalized device depth to window depth used by
    `DepthBufferMode::OPENGL_DEPTH`, as `glDepthRange`. By default [0, 1].
    @param nearValue window depth of the near plane
    @param farValue window depth of the far plane
    */
    void setOpenGlDepthRange(double nearValue, double farValue);

    /**
    @return window depth of the near plane for `DepthBufferMode::OPENGL_DEPTH`
    */
    double getOpenGlDepthRangeNear() const;

    /**
    @return window depth of the far plane for `DepthBufferMode::OPENGL_DEPTH`
    */
    double getOpenGlDepthRangeFar() const;

    /**
    @return depth buffer filled by the last `execute` call, of the size of
    its image, or null if the depth buffer mode is `NONE` or nothing was
    rendered yet. It is owned by this raytracer and reused (overwritten) by
    the next calls.
    */
    ZBuffer* getDepthBuffer() const;

    /**
    Raytraces the scene into the whole image, returning when it is complete.
    @param resultingImage image to fill; its size gives the resolution
    @param rendererConfiguration quality settings
    @param sceneSnapshot scene to render, seen from its camera snapshot
    @param reportProgress true to report in the console the number of
    threads and the progress
    */
    void execute(
        RGBImageUncompressed* resultingImage,
        const RendererConfiguration* rendererConfiguration,
        SimpleSceneSnapshot* sceneSnapshot,
        bool reportProgress);

    /**
    Raytraces the scene into the whole image and into a depth buffer given by
    the caller, independently of the configured depth buffer mode.
    @param resultingImage image to fill; its size gives the resolution
    @param outDepth depth buffer of the size of the image to fill
    @param depthMode kind of depth values to write; `NONE` leaves `outDepth`
    untouched
    @param rendererConfiguration quality settings
    @param sceneSnapshot scene to render, seen from its camera snapshot
    @param reportProgress true to report in the console the number of
    threads and the progress
    */
    void execute(
        RGBImageUncompressed* resultingImage,
        ZBuffer* outDepth,
        DepthBufferMode depthMode,
        const RendererConfiguration* rendererConfiguration,
        SimpleSceneSnapshot* sceneSnapshot,
        bool reportProgress);

    /**
    Stops the threads. The next `execute` creates them again.
    */
    void dispose();
};

#endif

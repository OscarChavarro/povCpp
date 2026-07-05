#include "render/RayShaderPipeline.h"
#include "render/RenderEngine.h"
#include "java/util/ArrayList.txx"

RenderWorker::RenderWorker(RenderEngine *owner)
    : engine(owner),
      primaryRay(nullptr),
      traceLevel(0),
      previousLine(nullptr),
      currentLine(nullptr),
      previousLineAntiAliasedFlags(nullptr),
      currentLineAntiAliasedFlags(nullptr),
      traceService(RenderWorker::traceServiceTrace,
          RenderWorker::traceServiceShadeShadow, this, &raySharedCache),
      textureUtils(nullptr)
{
}

RenderWorker::~RenderWorker()
{
    delete[] previousLine;
    delete[] currentLine;
    delete[] previousLineAntiAliasedFlags;
    delete[] currentLineAntiAliasedFlags;
}

ColorRgba *
RenderWorker::allocateColorBuffer(int count)
{
    return new ColorRgba[count];
}

void
RenderWorker::initializeLineBuffers(int width, bool antialiasEnabled)
{
    delete[] previousLine;
    delete[] currentLine;
    delete[] previousLineAntiAliasedFlags;
    delete[] currentLineAntiAliasedFlags;

    previousLine = allocateColorBuffer(width + 1);
    currentLine = allocateColorBuffer(width + 1);
    previousLineAntiAliasedFlags = nullptr;
    currentLineAntiAliasedFlags = nullptr;

    for (int i = 0; i <= width; i++) {
        previousLine[i].setR(0.0);
        previousLine[i].setG(0.0);
        previousLine[i].setB(0.0);

        currentLine[i].setR(0.0);
        currentLine[i].setG(0.0);
        currentLine[i].setB(0.0);
    }

    if (antialiasEnabled) {
        previousLineAntiAliasedFlags = new char[width + 1];
        currentLineAntiAliasedFlags = new char[width + 1];

        for (int i = 0; i <= width; i++) {
            previousLineAntiAliasedFlags[i] = 0;
            currentLineAntiAliasedFlags[i] = 0;
        }
    }

    primaryRay = &ray;
}

void
RenderWorker::swapLines()
{
    ColorRgba *tempColorPtr = previousLine;
    previousLine = currentLine;
    currentLine = tempColorPtr;

    char *tempCharPtr = previousLineAntiAliasedFlags;
    previousLineAntiAliasedFlags = currentLineAntiAliasedFlags;
    currentLineAntiAliasedFlags = tempCharPtr;
}

void
RenderWorker::traceServiceTrace(void *context, const RayWithTracingState *ray,
    ColorRgba *color)
{
    RenderWorker *worker = static_cast<RenderWorker *>(context);
    RayWithTracingState localRay = *ray;
    worker->engine->trace(*worker, &localRay, color);
}

void
RenderWorker::traceServiceShadeShadow(
    void *context, IntersectionCandidate *intersection, ColorRgba *color)
{
    RenderWorker *worker = static_cast<RenderWorker *>(context);
    RenderEngine &engine = *worker->engine;
    RayShaderPipeline::shadeSurface(
        intersection, color, nullptr, true, worker->getTraceService(),
        &worker->getTextureUtils(), engine.getRenderContext(), worker->traceLevel);
}

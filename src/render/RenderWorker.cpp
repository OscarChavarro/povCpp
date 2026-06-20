#include <new>

#include "java/util/ArrayList.txx"
#include "render/RayShaderPipeline.h"
#include "render/RenderEngine.h"
#include "render/RenderWorker.h"

RenderWorker::RenderWorker(RenderEngine *owner)
    : engine(owner),
      primaryRay(nullptr),
      traceLevel(0),
      previousLine(nullptr),
      currentLine(nullptr),
      previousLineAntiAliasedFlags(nullptr),
      currentLineAntiAliasedFlags(nullptr),
      traceService(RenderWorker::traceServiceTrace, RenderWorker::traceServiceAddColor,
          RenderWorker::traceServiceShadeShadow, this),
      activeTraceEvents(nullptr)
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
    ColorRgba *buffer = static_cast<ColorRgba *>(
        ::operator new[](sizeof(ColorRgba) * count));
    for (int i = 0; i < count; i++) {
        new (&buffer[i]) ColorRgba(0.0, 0.0, 0.0, 0.0);
    }
    return buffer;
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
RenderWorker::traceServiceTrace(void *context, const RayWithSegments *ray,
    const ColorRgba *multiplier)
{
    RenderWorker *worker = static_cast<RenderWorker *>(context);
    TraceEvent *event = new TraceEvent();
    event->childRay = true;
    event->ray = *ray;
    event->color = *multiplier;
    worker->activeTraceEvents->add(event);
}

void
RenderWorker::traceServiceAddColor(void *context, const ColorRgba *color)
{
    RenderWorker *worker = static_cast<RenderWorker *>(context);
    TraceEvent *event = new TraceEvent();
    event->childRay = false;
    event->color = *color;
    worker->activeTraceEvents->add(event);
}

void
RenderWorker::traceServiceShadeShadow(
    void *context, Intersection *intersection, ColorRgba *color)
{
    RenderWorker *worker = static_cast<RenderWorker *>(context);
    RenderEngine &engine = *worker->engine;
    RayShaderPipeline::shadeSurface(
        intersection, color, nullptr, true, worker->getTraceService(),
        &engine.getTextureUtils(), engine.getRenderContext(), worker->traceLevel);
}

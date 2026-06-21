#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"

#include "common/RenderRuntimeState.h"
#include "common/statistics/Statistics.h"
#include "environment/material/RendererConfiguration.h"
#include "render/AdaptiveAntiAliasing.h"
#include "render/ColorOperations.h"
#include "render/RenderEngine.h"
#include "render/RenderWorker.h"

inline unsigned short
AdaptiveAntiAliasing::rand3dInline(RenderWorker &worker, int a, int b)
{
    // worker's own TextureUtils (engine-shared in serial mode, task-private
    // in parallel mode) — never renderEngine->getTextureUtils() directly.
    ProceduralNoise &noise = worker.getTextureUtils().getProceduralNoise();
    return noise.checksumTable().eval((int)(noise.hashTable()[(int)(noise.hashTable()[(int)(a & 0xfff)] ^ b) &
                                  0xfff]));
}

void
AdaptiveAntiAliasing::superSample(
    RenderWorker &worker, ColorRgba *result, int x, int y, int width, int height,
    IntersectionPriorityQueuePool *pool, Statistics *stats)
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

    stats->incrementNumberOfPixelsSuperSampled();

    result->setR(0.0); result->setG(0.0); result->setB(0.0); result->setA(0);

    for (int sampleIndex = 0; sampleIndex < SUPER_SAMPLE_COUNT; sampleIndex++) {
        const int jitterSeedY = sampleIndex == 0 ? y : y + jitterSeedOffset;
        const double jitterX =
            (rand3dInline(worker, x + jitterSeedOffset, jitterSeedY) & JITTER_RANDOM_MASK) /
                JITTER_RANDOM_NORMALIZER * SUPER_SAMPLE_JITTER_RANGE -
            SUPER_SAMPLE_JITTER_BIAS;
        const double jitterY =
            (rand3dInline(worker, x + jitterSeedOffset, jitterSeedY) & JITTER_RANDOM_MASK) /
                JITTER_RANDOM_NORMALIZER * SUPER_SAMPLE_JITTER_RANGE -
            SUPER_SAMPLE_JITTER_BIAS;

        renderEngine->createRay(worker.getPrimaryRay(), width, height,
            dx + jitterX + superSampleOffsets[sampleIndex][0],
            dy + jitterY + superSampleOffsets[sampleIndex][1], pool, stats);
        worker.setTraceLevel(0);
        renderEngine->trace(worker, worker.getPrimaryRay(), &color);
        ColorOperations::clipColor(&color, &color);
        ColorOperations::scaleColor(&color, &color, SUPER_SAMPLE_WEIGHT);
        ColorOperations::addColor(result, result, &color);

        jitterSeedOffset += JITTER_SEED_INCREMENT;
    }

    if ((y != renderEngine->getConfig().getFirstLine() - 1) &&
        renderEngine->getConfig().hasOptionFlags(RenderingConfiguration::DISPLAY)) {
    }
}

void
AdaptiveAntiAliasing::doAntiAliasing(RenderWorker &worker, int x, int y,
    ColorRgba *color, const RasterTileArea &area,
    IntersectionPriorityQueuePool *pool, Statistics *stats)
{
    char antialiasCenterFlag = 0;

    worker.setCurrentLineAntiAliasedFlag(x, false);

    if (x != area.getX0()) {
        // M1 (future, symmetric to the row clip below): for vertical-strip
        // tiles this would need (x - 1) >= area.getX0(), already true here
        // since the branch only runs when x != area.getX0(). Not needed for
        // horizontal bands, where x0 == 0 for every tile.
        if (ColorOperations::colorDistance(
                worker.getCurrentLinePixel(x - 1),
                worker.getCurrentLinePixel(x)) >=
            renderEngine->getScene().getAntialiasThreshold()) {
            antialiasCenterFlag = 1;
            if (!worker.getCurrentLineAntiAliasedFlag(x - 1)) {
                superSample(worker, worker.getCurrentLinePixel(x - 1), x - 1, y,
                    renderEngine->getScene().getScreenWidth(),
                    renderEngine->getScene().getScreenHeight(), pool, stats);
                worker.setCurrentLineAntiAliasedFlag(x - 1, true);
            }
        }
    }

    if (y != area.getY0() - 1) {
        if (ColorOperations::colorDistance(
                worker.getPreviousLinePixel(x),
                worker.getCurrentLinePixel(x)) >=
            renderEngine->getScene().getAntialiasThreshold()) {
            antialiasCenterFlag = 1;
            // M1: never write a supersampled pixel into a row owned by
            // another tile (the row above this task's RasterTileArea).
            if (!worker.getPreviousLineAntiAliasedFlag(x) &&
                (y - 1) >= area.getY0()) {
                superSample(worker, worker.getPreviousLinePixel(x), x, y - 1,
                    renderEngine->getScene().getScreenWidth(),
                    renderEngine->getScene().getScreenHeight(), pool, stats);
                worker.setPreviousLineAntiAliasedFlag(x, true);
            }
        }
    }

    if (antialiasCenterFlag) {
        superSample(worker, worker.getCurrentLinePixel(x), x, y,
            renderEngine->getScene().getScreenWidth(),
            renderEngine->getScene().getScreenHeight(), pool, stats);
        worker.setCurrentLineAntiAliasedFlag(x, true);
        *color = *worker.getCurrentLinePixel(x);
    }
}

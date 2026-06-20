#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"

#include "common/RenderRuntimeState.h"
#include "common/statistics/Statistics.h"
#include "environment/material/RendererConfiguration.h"
#include "render/AdaptiveAntiAliasing.h"
#include "render/ColorOperations.h"
#include "render/RenderEngine.h"
#include "render/RenderWorker.h"

inline unsigned short
AdaptiveAntiAliasing::rand3dInline(int a, int b)
{
    ProceduralNoise &noise = renderEngine->getTextureUtils().getProceduralNoise();
    return noise.checksumTable().eval((int)(noise.hashTable()[(int)(noise.hashTable()[(int)(a & 0xfff)] ^ b) &
                                  0xfff]));
}

void
AdaptiveAntiAliasing::superSample(
    RenderWorker &worker, ColorRgba *result, int x, int y, int width, int height)
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

    renderEngine->getStatistics().incrementNumberOfPixelsSupersampled();

    result->setR(0.0); result->setG(0.0); result->setB(0.0); result->setA(0);

    for (int sampleIndex = 0; sampleIndex < SUPER_SAMPLE_COUNT; sampleIndex++) {
        const int jitterSeedY = sampleIndex == 0 ? y : y + jitterSeedOffset;
        const double jitterX =
            (rand3dInline(x + jitterSeedOffset, jitterSeedY) & JITTER_RANDOM_MASK) /
                JITTER_RANDOM_NORMALIZER * SUPER_SAMPLE_JITTER_RANGE -
            SUPER_SAMPLE_JITTER_BIAS;
        const double jitterY =
            (rand3dInline(x + jitterSeedOffset, jitterSeedY) & JITTER_RANDOM_MASK) /
                JITTER_RANDOM_NORMALIZER * SUPER_SAMPLE_JITTER_RANGE -
            SUPER_SAMPLE_JITTER_BIAS;

        renderEngine->createRay(worker.getPrimaryRay(), width, height,
            dx + jitterX + superSampleOffsets[sampleIndex][0],
            dy + jitterY + superSampleOffsets[sampleIndex][1]);
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
AdaptiveAntiAliasing::doAntiAliasing(RenderWorker &worker, int x, int y, ColorRgba *color)
{
    char antialiasCenterFlag = 0;

    worker.setCurrentLineAntiAliasedFlag(x, false);

    if (x != 0) {
        if (ColorOperations::colorDistance(
                worker.getCurrentLinePixel(x - 1),
                worker.getCurrentLinePixel(x)) >=
            renderEngine->getScene().getAntialiasThreshold()) {
            antialiasCenterFlag = 1;
            if (!worker.getCurrentLineAntiAliasedFlag(x - 1)) {
                superSample(worker, worker.getCurrentLinePixel(x - 1), x - 1, y,
                    renderEngine->getScene().getScreenWidth(),
                    renderEngine->getScene().getScreenHeight());
                worker.setCurrentLineAntiAliasedFlag(x - 1, true);
                renderEngine->incrementSuperSampleCount();
            }
        }
    }

    if (y != renderEngine->getConfig().getFirstLine() - 1) {
        if (ColorOperations::colorDistance(
                worker.getPreviousLinePixel(x),
                worker.getCurrentLinePixel(x)) >=
            renderEngine->getScene().getAntialiasThreshold()) {
            antialiasCenterFlag = 1;
            if (!worker.getPreviousLineAntiAliasedFlag(x)) {
                superSample(worker, worker.getPreviousLinePixel(x), x, y - 1,
                    renderEngine->getScene().getScreenWidth(),
                    renderEngine->getScene().getScreenHeight());
                worker.setPreviousLineAntiAliasedFlag(x, true);
                renderEngine->incrementSuperSampleCount();
            }
        }
    }

    if (antialiasCenterFlag) {
        superSample(worker, worker.getCurrentLinePixel(x), x, y,
            renderEngine->getScene().getScreenWidth(),
            renderEngine->getScene().getScreenHeight());
        worker.setCurrentLineAntiAliasedFlag(x, true);
        *color = *worker.getCurrentLinePixel(x);
        renderEngine->incrementSuperSampleCount();
    }
}

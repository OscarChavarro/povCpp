/**
Implements texture-space transforms (translate/rotate/scale/copy) for POV-Ray
material descriptors, plus the global default texture and color-map sampling.
The Perlin noise primitives (Noise, DNoise, Turbulence, DTurbulence, cycloidal,
triangleWave) are implemented in ProceduralNoise. Color, bump, and map texture
routines are in ColorTextureFixture.cpp, BumpTextureFixture.cpp, and
MapTextureFixture.cpp respectively.
*/

#include <cstdlib>
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/statistics/SolidTextureStatistics.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"

namespace {
constexpr long kWaveRandomMask = 0x7FFF;
constexpr float kWaveRandomDivisor = static_cast<float>(kWaveRandomMask);
}

TextureUtils* TextureUtils::sActive = nullptr;

TextureUtils::TextureUtils(SolidTextureStatistics *stats)
    : proceduralNoise(new ProceduralNoise(stats))
{
}

void
TextureUtils::initialize(SolidTextureStatistics *stats)
{
    if (sActive) {
        delete sActive->proceduralNoise;
        sActive->proceduralNoise = new ProceduralNoise(stats);
    }
}

ProceduralNoise&
TextureUtils::getProceduralNoise()
{
    if (!proceduralNoise) {
        proceduralNoise = new ProceduralNoise();
    }
    return *proceduralNoise;
}

double
TextureUtils::floorInline(double x)
{
    return x >= 0.0 ? java::Math::floor(x) : (0.0 - java::Math::floor(0.0 - x) - 1.0);
}

double
TextureUtils::fabsInline(double x)
{
    return (x < 0.0) ? (0.0 - x) : x;
}

void
TextureUtils::computeColor(
    ColorRgba *color, const RGBAColorPalette *colorMap, double value)
{
    const ColorRgba *c = colorMap->evalLinear(value);
    *color = *c;
    delete c;
}

void
TextureUtils::initializeNoise(int numberOfWaves)
{
    Vector3Dd point;

    proceduralNoise->initialize();

    frequencyInstance.clear();
    waveSourcesInstance.clear();
    frequencyInstance.reserve(numberOfWaves);
    waveSourcesInstance.reserve(numberOfWaves);

    for (int i = 0; i < numberOfWaves; i++) {
        frequencyInstance.add(0.0);
        waveSourcesInstance.add(Vector3Dd());
    }

    for (int i = 0; i < numberOfWaves; i++) {
        proceduralNoise->differentialNoise(&point, (double)i, 0.0, 0.0);
        waveSources()[i] = point.normalizedFast();
        waveFrequency()[i] = (rand() & kWaveRandomMask) / kWaveRandomDivisor + 0.01;
    }
}

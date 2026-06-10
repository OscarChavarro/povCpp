/**
Implements texture-space transforms (translate/rotate/scale/copy) for POV-Ray
material descriptors, plus the global default texture and color-map sampling.
The Perlin noise primitives (Noise, DNoise, Turbulence, DTurbulence, cycloidal,
triangleWave) are implemented in ProceduralNoise. Color, bump, and map texture
routines are in ColorTextureFixture.cpp, BumpTextureFixture.cpp, and
MapTextureFixture.cpp respectively.
*/

#include <cstdlib>

#include "common/statistics/SolidTextureStatistics.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "solidTexture/TextureUtils.h"

static double frequencyInstance[TextureUtils::NUMBER_OF_WAVES];
static Vector3Dd waveSourcesInstance[TextureUtils::NUMBER_OF_WAVES];

TextureUtils* TextureUtils::textureInstance = nullptr;

TextureUtils::TextureUtils(SolidTextureStatistics *stats)
    : proceduralNoise_(stats)
{
}

void
TextureUtils::initialize(SolidTextureStatistics *stats)
{
    static TextureUtils inst(stats);
    textureInstance = &inst;
}

TextureUtils&
TextureUtils::instance()
{
    return *textureInstance;
}

ProceduralNoise&
TextureUtils::proceduralNoise()
{
    return proceduralNoise_;
}

double *
TextureUtils::waveFrequency()
{
    return frequencyInstance;
}

Vector3Dd *
TextureUtils::waveSources()
{
    return waveSourcesInstance;
}

double
TextureUtils::floorInline(double x)
{
    return (x >= 0.0) ? floor(x) : (0.0 - floor(0.0 - x) - 1.0);
}

double
TextureUtils::fabsInline(double x)
{
    return (x < 0.0) ? (0.0 - x) : x;
}

void
TextureUtils::computeColor(
    ColorRgba *color, RGBAColorPalette *colorMap, double value)
{
    ColorRgba *c = colorMap->evalLinear(value);
    *color = *c;
    delete c;
}

void
TextureUtils::initializeNoise()
{
    int i;
    Vector3Dd point;

    proceduralNoise_.initialize();

    for (i = 0; i < TextureUtils::NUMBER_OF_WAVES; i++) {
        proceduralNoise_.dNoise(&point, (double)i, 0.0, 0.0);
        waveSources()[i] = point.normalizedFast();
        waveFrequency()[i] = (rand() & ProceduralNoise::RNDMASK) / ProceduralNoise::RND_DIVISOR + 0.01;
    }
}

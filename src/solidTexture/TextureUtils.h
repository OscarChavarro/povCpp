/**
The Perlin noise primitives (Noise, DNoise, Turbulence, DTurbulence, cycloidal,
triangleWave) live in ProceduralNoise, accessible via proceduralNoise().
*/

#ifndef __TEXTURE_UTILS_H__
#define __TEXTURE_UTILS_H__

#include "common/statistics/SolidTextureStatistics.h"
#include "solidTexture/procedural/ProceduralNoise.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"

class TextureUtils {
  public:
    static constexpr int NUMBER_OF_WAVES = 10;

    static void initialize(SolidTextureStatistics *stats);
    static TextureUtils& instance();

    ProceduralNoise& getProceduralNoise();

    double floorInline(double x);
    double fabsInline(double x);
    double *waveFrequency();
    Vector3Dd *waveSources();
    void computeColor(ColorRgba *color, RGBAColorPalette *colorMap, double value);
    void initializeNoise();

  private:
    ProceduralNoise proceduralNoise;
    static TextureUtils* textureInstance;
    explicit TextureUtils(SolidTextureStatistics *stats);
};

#endif

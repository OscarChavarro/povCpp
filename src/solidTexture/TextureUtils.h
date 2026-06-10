/**
Texture-side utilities: the global default texture, color-map sampling, the wave
sources/frequencies used by ripple/wave bump textures, and texture-space transforms
(translate/rotate/scale/copy) for POV-Ray material descriptors.

The Perlin noise primitives (Noise, DNoise, Turbulence, DTurbulence, cycloidal,
triangleWave) live in ProceduralNoise, accessible via proceduralNoise().
*/

#ifndef __TEXTURE_UTILS_H__
#define __TEXTURE_UTILS_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "solidTexture/ProceduralNoise.h"

class RGBAColorPalette;
class Texture;

class TextureUtils {
  public:
    static void initialize(SolidTextureStatistics *stats);
    static TextureUtils& instance();

    ProceduralNoise& proceduralNoise();

    double floorInline(double x);
    double fabsInline(double x);
    Texture *&defaultTexture();
    double *waveFrequency();
    Vector3Dd *waveSources();
    void computeColor(ColorRgba *color, RGBAColorPalette *colorMap, double value);
    void initializeNoise(void);
    void translateTexture(Texture **Texture_Ptr, Vector3Dd *Vector);
    void rotateTexture(Texture **Texture_Ptr, Vector3Dd *Vector);
    void scaleTexture(Texture **Texture_Ptr, Vector3Dd *Vector);
    Texture *copyTexture(Texture *texture);
    Texture *getTexture();

  private:
    ProceduralNoise proceduralNoise_;
    static TextureUtils* inst_;
    TextureUtils(SolidTextureStatistics *stats);
};

#endif

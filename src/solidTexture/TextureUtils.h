#ifndef __TEXTURE_UTILS_H__
#define __TEXTURE_UTILS_H__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "common/statistics/SolidTextureStatistics.h"
#include "solidTexture/procedural/ProceduralNoise.h"

class TextureUtils {
  private:
    ProceduralNoise proceduralNoise;
    static TextureUtils* textureInstance;
    static java::ArrayList<double> frequencyInstance;
    static java::ArrayList<Vector3Dd> waveSourcesInstance;
    explicit TextureUtils(SolidTextureStatistics *stats);

  public:
    static void initialize(SolidTextureStatistics *stats);
    static TextureUtils& instance();
    ProceduralNoise& getProceduralNoise();
    static double floorInline(double x);
    static double fabsInline(double x);
    static double *waveFrequency();
    static Vector3Dd *waveSources();
    static void computeColor(ColorRgba *color, const RGBAColorPalette *colorMap, double value);
    void initializeNoise(int numberOfWaves);
};

#endif

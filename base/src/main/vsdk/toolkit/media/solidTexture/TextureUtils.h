#ifndef __TEXTURE_UTILS_H__
#define __TEXTURE_UTILS_H__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/statistics/SolidTextureStatistics.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "vsdk/toolkit/media/solidTexture/procedural/ProceduralNoise.h"
class TextureUtils {
  private:
    ProceduralNoise* proceduralNoise;
    java::ArrayList<double> frequencyInstance;
    java::ArrayList<Vector3Dd> waveSourcesInstance;
    explicit TextureUtils(SolidTextureStatistics *stats);

  public:
    TextureUtils() : proceduralNoise(nullptr) {}
    ~TextureUtils() { delete proceduralNoise; }

    void initialize(SolidTextureStatistics *stats);
    ProceduralNoise& getProceduralNoise();
    static double floorInline(double x);
    static double fabsInline(double x);
    inline double *waveFrequency() { return frequencyInstance.data(); }
    inline Vector3Dd *waveSources() { return waveSourcesInstance.data(); }
    static void computeColor(ColorRgba *color, const RGBAColorPalette *colorMap, double value);
    void initializeNoise(int numberOfWaves);
};

#endif

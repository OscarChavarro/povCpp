/**
[PERL1985] - Ken Perlin, "An Image Synthesizer", SIGGRAPH '85.
Core Perlin noise primitives: Noise() and DNoise() functions for solid texturing.
See Texture.cpp for detailed implementations of lattice-based interpolation,
pseudorandom gradients via hash tables, and s-curve smoothing.
*/

#ifndef __TEXTURE_UTILS_H__
#define __TEXTURE_UTILS_H__

#include "common/color/ColorOperations.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class RGBAColorPalette;
class Texture;
class SolidTextureStatistics;

class textureUtils {
  public:
    static void initialize(SolidTextureStatistics* stats);
    static textureUtils& instance();

    double floorInline(double x);
    double fabsInline(double x);
    double sCurve(double a);
    short hash3d(long a, long b, long c);
    double incrSum(int m, double s, double x, double y, double z);
    Texture *&defaultTexture();
    double *&rTable();
    short *&hashTable();
    double *&sinTable();
    double *waveFrequency();
    Vector3Dd *waveSources();
    unsigned short *crcTable();
    void computeColor(ColorRgba *color, RGBAColorPalette *colorMap, double value);
    void initializeNoise(void);
    void InitTextureTable(void);
    void InitRTable(void);
    int R(Vector3Dd *v);
    int Crc16(char *buf, int count);
    double Noise(double x, double y, double z);
    void DNoise(Vector3Dd *result, double x, double y, double z);
    double cycloidal(double value);
    double triangleWave(double value);
    double Turbulence(double x, double y, double z, int octaves);
    void DTurbulence(Vector3Dd *result, double x, double y, double z, int octaves);
    void translateTexture(Texture **Texture_Ptr, Vector3Dd *Vector);
    void rotateTexture(Texture **Texture_Ptr, Vector3Dd *Vector);
    void scaleTexture(Texture **Texture_Ptr, Vector3Dd *Vector);
    Texture *copyTexture(Texture *texture);
    Texture *getTexture();

  private:
    SolidTextureStatistics* solidTextureStats_;
    static textureUtils* inst_;
    explicit textureUtils(SolidTextureStatistics* stats);
};

#endif

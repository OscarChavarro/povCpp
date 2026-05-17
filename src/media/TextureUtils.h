#ifndef __TEXTURE_UTILS_H__
#define __TEXTURE_UTILS_H__

#include "common/Color.h"
#include "common/linealAlgebra/Vector3Dd.h"

class RGBAColorPalette;
class Texture;

class TextureUtils {
  public:
    static void computeColour(
        RGBAColor *colour, RGBAColorPalette *colourMap, double value);
    static void initializeNoise(void);
    static void InitTextureTable(void);
    static void InitRTable(void);
    static int R(Vector3Dd *v);
    static int Crc16(char *buf, int count);
    static double Noise(double x, double y, double z);
    static void DNoise(Vector3Dd *result, double x, double y, double z);
    static double cycloidal(double value);
    static double triangleWave(double value);
    static double Turbulence(double x, double y, double z, int octaves);
    static void DTurbulence(
        Vector3Dd *result, double x, double y, double z, int octaves);
    static void translateTexture(Texture **Texture_Ptr, Vector3Dd *Vector);
    static void rotateTexture(Texture **Texture_Ptr, Vector3Dd *Vector);
    static void scaleTexture(Texture **Texture_Ptr, Vector3Dd *Vector);
    static Texture *getTexture();
};

#endif

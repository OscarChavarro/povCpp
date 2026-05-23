#ifndef __TEXTURE_H__
#define __TEXTURE_H__
/****************************************************************************
 *                         texture.h
 *
 *  This file contains defines and variables for the txt*.c files
 *
 *
 *****************************************************************************/

#include "common/color/Color.h"
#include "common/LegacyBoolean.h"
#include "common/linealAlgebra/Transformation.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "media/RGBAImage.h"
#include "media/RGBAPixel.h"
#include "media/TextureUtils.h"


class Texture {
  public:
    Texture *Next_Texture;
    Texture *Next_Material;
    int numberOfMaterials;
    double objectReflection;
    double objectAmbient;
    double objectDiffuse;
    double objectBrilliance;
    double objectIndexOfRefraction;
    double objectRefraction;
    double objectTransmit;
    double objectSpecular;
    double objectRoughness;
    double objectPhong;
    double objectPhongSize;
    double bumpAmount;
    double textureRandomness;
    double Frequency;
    double Phase;
    int textureNumber;
    int bumpNumber;
    int textureIndex;
    Transformation *Texture_Transformation;
    RGBAColor *Colour1;
    RGBAColor *Colour2;
    double Turbulence;
    Vector3Dd textureGradient;
    RGBAColorPalette *Colour_Map;
    RGBAImage *Image;
    RGBAImage *Bump_Image;
    RGBAImage *Material_Image;
    short metallicFlag;
    short onceFlag;
    short constantFlag;
    int Octaves;   /* dmf, 1/92 for turb */
    double Mortar; /* rha, 2/92 for brick */

    static inline double
    floorInline(double x)
    {
        return (x >= 0.0) ? floor(x) : (0.0 - floor(0.0 - x) - 1.0);
    }
    static inline double
    fabsInline(double x)
    {
        return (x < 0.0) ? (0.0 - x) : x;
    }
    static inline double
    sCurve(double a)
    {
        return a * a * (3.0 - 2.0 * a);
    }
};

/* Image/Bump Map projection methods */
static constexpr int PLANAR_MAP = 0;
static constexpr int SPHERICAL_MAP = 1;
static constexpr int CYLINDRICAL_MAP = 2;
static constexpr int PARABOLIC_MAP = 3;
static constexpr int HYPERBOLIC_MAP = 4;
static constexpr int TORUS_MAP = 5;
static constexpr int PIRIFORM_MAP = 6;
static constexpr int OLD_MAP = 7;

/* Bit map interpolation types */
static constexpr int NO_INTERPOLATION = 0;
static constexpr int NEAREST_NEIGHBOR = 1;
static constexpr int BILINEAR = 2;
static constexpr int CUBIC_SPLINE = 3;
static constexpr int NORMALIZED_DIST = 4;

/* Coloration texture list */
static constexpr int NO_TEXTURE = 0;
static constexpr int COLOUR_TEXTURE = 1;
static constexpr int BOZO_TEXTURE = 2;
static constexpr int MARBLE_TEXTURE = 3;
static constexpr int WOOD_TEXTURE = 4;
static constexpr int CHECKER_TEXTURE = 5;
static constexpr int CHECKER_TEXTURE_TEXTURE = 6;
static constexpr int SPOTTED_TEXTURE = 7;
static constexpr int AGATE_TEXTURE = 8;
static constexpr int GRANITE_TEXTURE = 9;
static constexpr int GRADIENT_TEXTURE = 10;
static constexpr int IMAGEMAP_TEXTURE = 11;
static constexpr int PAINTED1_TEXTURE = 12;
static constexpr int PAINTED2_TEXTURE = 13;
static constexpr int PAINTED3_TEXTURE = 14;
static constexpr int ONION_TEXTURE = 15;
static constexpr int LEOPARD_TEXTURE = 16;
static constexpr int BRICK_TEXTURE = 17; /* RHA 2/92 for brick */
static constexpr int MATERIAL_MAP_TEXTURE =
    99; /* Not really colored, but... CdW */

/* Normal perturbation (bumpy) texture list  */
static constexpr int NO_BUMPS = 0;
static constexpr int WAVES = 1;
static constexpr int RIPPLES = 2;
static constexpr int WRINKLES = 3;
static constexpr int BUMPS = 4;
static constexpr int DENTS = 5;
static constexpr int BUMPY1 = 6;
static constexpr int BUMPY2 = 7;
static constexpr int BUMPY3 = 8;
static constexpr int BUMPMAP = 9;

static constexpr int MINX = -10000; /* Ridiculously large scaling values */
static constexpr int MINY = MINX;
static constexpr int MINZ = MINX;

static constexpr int MAXSIZE = 267;
static constexpr long RNDMASK = 0x7FFF;
static constexpr float rndDivisor = static_cast<float>(RNDMASK);
static constexpr int NUMBER_OF_WAVES = 10;
static constexpr int SINTABSIZE = 1000;

static constexpr double realScale = (2.0 / 65535.0);

// Deprecated: Use Texture:: methods instead
inline double
floorInline(double x)
{
    return Texture::floorInline(x);
}
inline double
fabsInline(double x)
{
    return Texture::fabsInline(x);
}
inline double
sCurve(double a)
{
    return Texture::sCurve(a);
}

// These need to be global because they access external variables
inline short
hash3d(long a, long b, long c)
{
    return TextureUtils::hashTable()[(
        int)(TextureUtils::hashTable()[(int)(TextureUtils::hashTable()[(int)(a & 0xfffL)] ^ (b & 0xfffL))] ^
             (c & 0xfffL))];
}

inline double
incrSum(int m, double s, double x, double y, double z)
{
    return s * (TextureUtils::rTable()[m] * 0.5 + TextureUtils::rTable()[m + 1] * x + TextureUtils::rTable()[m + 2] * y +
                   TextureUtils::rTable()[m + 3] * z);
}

#endif

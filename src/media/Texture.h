#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include "common/color/Color.h"
#include "common/linealAlgebra/Transformation.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "media/RGBAImage.h"
#include "media/RGBAPixel.h"
#include "media/TextureUtils.h"


class Texture {
  public:
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
    bool metallicFlag;
    bool onceFlag;
    bool constantFlag;
    int Octaves;   /* dmf, 1/92 for turb */
    double Mortar; /* rha, 2/92 for brick */
};

#endif

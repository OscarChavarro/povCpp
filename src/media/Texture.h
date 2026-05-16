#ifndef __TEXTURE_H__
#define __TEXTURE_H__
/****************************************************************************
 *                         texture.h
 *
 *  This file contains defines and variables for the txt*.c files
 *
 *
 *****************************************************************************/

#include "common/Colour.h"
#include "common/Frame.h"
#include "common/Matrices.h"
#include "common/Vector.h"

extern long callsToNoise;
extern long callsToDNoise;

/* Types for reading IFF files. */
class RGBAPixel {
  public:
    unsigned short Red, Green, Blue, Alpha;
};

class ImageLine {
  public:
    unsigned char *red, *green, *blue;
};

class RGBAImage {
  public:
    class ImageData {
      public:
        ImageLine *rgb_lines;
        unsigned char **map_lines;
    };

    DBL width, height;
    int iwidth, iheight;
    int Map_Type;
    int Interpolation_Type;
    short Once_Flag;
    short Use_Colour_Flag;
    Vector3D Image_Gradient;
    short Colour_Map_Size;
    RGBAPixel *Colour_Map;
    ImageData data;
};

class Texture {
  public:
    Texture *Next_Texture;
    Texture *Next_Material;
    int Number_Of_Materials;
    DBL Object_Reflection;
    DBL Object_Ambient;
    DBL Object_Diffuse, Object_Brilliance;
    DBL Object_Index_Of_Refraction;
    DBL Object_Refraction, Object_Transmit;
    DBL Object_Specular, Object_Roughness;
    DBL Object_Phong, Object_PhongSize;
    DBL Bump_Amount;
    DBL Texture_Randomness;
    DBL Frequency;
    DBL Phase;
    int Texture_Number;
    int Bump_Number;
    int Texture_Index;
    Transformation *Texture_Transformation;
    RGBAColor *Colour1;
    RGBAColor *Colour2;
    DBL Turbulence;
    Vector3D Texture_Gradient;
    RGBAColorPalette *Colour_Map;
    RGBAImage *Image;
    RGBAImage *Bump_Image;
    RGBAImage *Material_Image;
    short Metallic_Flag, Once_Flag, Constant_Flag;
    int Octaves; /* dmf, 1/92 for turb */
    DBL Mortar;  /* rha, 2/92 for brick */
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
static constexpr int MATERIAL_MAP_TEXTURE = 99; /* Not really colored, but... CdW */

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

extern DBL *RTable;
extern short *hashTable;

inline DBL floorInline(DBL x)
{
    return (x >= 0.0) ? floor(x) : (0.0 - floor(0.0 - x) - 1.0);
}

inline DBL fabsInline(DBL x)
{
    return (x < 0.0) ? (0.0 - x) : x;
}

inline DBL sCurve(DBL a)
{
    return a * a * (3.0 - 2.0 * a);
}
static constexpr double realScale = (2.0 / 65535.0);
inline short hash3d(long a, long b, long c)
{
    return hashTable[(int)(hashTable[(int)(hashTable[(int)(a & 0xfffL)] ^
                                      (b & 0xfffL))] ^
                           (c & 0xfffL))];
}

inline DBL incrSum(int m, DBL s, DBL x, DBL y, DBL z)
{
    return s * (RTable[m] * 0.5 + RTable[m + 1] * x + RTable[m + 2] * y +
                RTable[m + 3] * z);
}

extern DBL *sintab;
extern DBL frequency[NUMBER_OF_WAVES];
extern Vector3D Wave_Sources[NUMBER_OF_WAVES];
extern DBL *RTable;
extern short *hashTable;
extern unsigned short crctab[256];
extern Texture *Default_Texture;

extern void computeColour(
    RGBAColor *Colour, RGBAColorPalette *Colour_Map, DBL value);
extern void initializeNoise(void);
extern void InitTextureTable(void);
extern void InitRTable(void);
extern int R(Vector3D *v);
extern int Crc16(char *buf, int count);
extern void setupLattice(DBL *x, DBL *y, DBL *z, long *ix, long *iy, long *iz,
    long *jx, long *jy, long *jz, DBL *sx, DBL *sy, DBL *sz, DBL *tx, DBL *ty,
    DBL *tz);
extern DBL Noise(DBL x, DBL y, DBL z);
extern void DNoise(Vector3D *result, DBL x, DBL y, DBL z);
extern DBL cycloidal(DBL value);
extern DBL triangleWave(DBL value);
extern DBL Turbulence(DBL x, DBL y, DBL z, int octaves);
extern void DTurbulence(Vector3D *result, DBL x, DBL y, DBL z, int octaves);
extern void translateTexture(Texture **Texture_Ptr, Vector3D *Vector);
extern void rotateTexture(Texture **Texture_Ptr, Vector3D *Vector);
extern void scaleTexture(Texture **Texture_Ptr, Vector3D *Vector);

extern Texture *copyTexture(Texture *Texture);
extern Texture *getTexture();

#endif

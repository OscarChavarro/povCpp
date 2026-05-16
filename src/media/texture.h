#ifndef __TEXTURE_H__
#define __TEXTURE_H__
/****************************************************************************
 *                         texture.h
 *
 *  This file contains defines and variables for the txt*.c files
 *
 *
 *  from Persistence of Vision Raytracer
 *  Copyright 1992 Persistence of Vision Team
 *---------------------------------------------------------------------------
 *  Copying, distribution and legal info is in the file povlegal.doc which
 *  should be distributed with this file. If povlegal.doc is not available
 *  or for more info please contact:
 *
 *         Drew Wells [POV-Team Leader]
 *         CIS: 73767,1244  Internet: 73767.1244@compuserve.com
 *         Phone: (213) 254-4041
 *
 * This program is based on the popular DKB raytracer version 2.12.
 * DKBTrace was originally written by David K. Buck.
 * DKBTrace Ver 2.0-2.12 were written by David K. Buck & Aaron A. Collins.
 *
 *****************************************************************************/

#include "common/frame.h"
#include "common/colour.h"
#include "common/matrices.h"
#include "common/vector.h"

extern long Calls_To_Noise, Calls_To_DNoise;

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
    DBL width, height;
    int iwidth, iheight;
    int Map_Type;
    int Interpolation_Type;
    short Once_Flag;
    short Use_Colour_Flag;
    Vector3D Image_Gradient;
    short Colour_Map_Size;
    RGBAPixel *Colour_Map;
    union {
        ImageLine *rgb_lines;
        unsigned char **map_lines;
    } data;
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
#define PLANAR_MAP 0
#define SPHERICAL_MAP 1
#define CYLINDRICAL_MAP 2
#define PARABOLIC_MAP 3
#define HYPERBOLIC_MAP 4
#define TORUS_MAP 5
#define PIRIFORM_MAP 6
#define OLD_MAP 7

/* Bit map interpolation types */
#define NO_INTERPOLATION 0
#define NEAREST_NEIGHBOR 1
#define BILINEAR 2
#define CUBIC_SPLINE 3
#define NORMALIZED_DIST 4

/* Coloration texture list */
#define NO_TEXTURE 0
#define COLOUR_TEXTURE 1
#define BOZO_TEXTURE 2
#define MARBLE_TEXTURE 3
#define WOOD_TEXTURE 4
#define CHECKER_TEXTURE 5
#define CHECKER_TEXTURE_TEXTURE 6
#define SPOTTED_TEXTURE 7
#define AGATE_TEXTURE 8
#define GRANITE_TEXTURE 9
#define GRADIENT_TEXTURE 10
#define IMAGEMAP_TEXTURE 11
#define PAINTED1_TEXTURE 12
#define PAINTED2_TEXTURE 13
#define PAINTED3_TEXTURE 14
#define ONION_TEXTURE 15
#define LEOPARD_TEXTURE 16
#define BRICK_TEXTURE 17        /* RHA 2/92 for brick */
#define MATERIAL_MAP_TEXTURE 99 /* Not really colored, but... CdW */

/* Normal perturbation (bumpy) texture list  */
#define NO_BUMPS 0
#define WAVES 1
#define RIPPLES 2
#define WRINKLES 3
#define BUMPS 4
#define DENTS 5
#define BUMPY1 6
#define BUMPY2 7
#define BUMPY3 8
#define BUMPMAP 9

#define MINX -10000 /* Ridiculously large scaling values */
#define MINY MINX
#define MINZ MINX

#define MAXSIZE 267
#define RNDMASK 0x7FFF
#define RNDDIVISOR (float)RNDMASK
#define NUMBER_OF_WAVES 10
#define SINTABSIZE 1000

#define FLOOR(x) ((x) >= 0.0 ? floor(x) : (0.0 - floor(0.0 - (x)) - 1.0))
#define FABS(x) ((x) < 0.0 ? (0.0 - x) : (x))
#define SCURVE(a) ((a) * (a) * (3.0 - 2.0 * (a)))
#define REALSCALE (2.0 / 65535.0)
#define Hash3d(a, b, c)                                                        \
    hashTable[(                                                                \
        int)(hashTable[(int)(hashTable[(int)((a)&0xfffL)] ^ ((b)&0xfffL))] ^   \
             ((c)&0xfffL))]
#define INCRSUM(m, s, x, y, z)                                                 \
    ((s) * (RTable[m] * 0.5 + RTable[m + 1] * (x) + RTable[m + 2] * (y) +      \
               RTable[m + 3] * (z)))

extern DBL *sintab;
extern DBL frequency[NUMBER_OF_WAVES];
extern Vector3D Wave_Sources[NUMBER_OF_WAVES];
extern DBL *RTable;
extern short *hashTable;
extern unsigned short crctab[256];
extern Texture *Default_Texture;

extern void Compute_Colour(
    RGBAColor *Colour, RGBAColorPalette *Colour_Map, DBL value);
extern void Initialize_Noise(void);
extern void InitTextureTable(void);
extern void InitRTable(void);
extern int R(Vector3D *v);
extern int Crc16(char *buf, int count);
extern void setup_lattice(DBL *x, DBL *y, DBL *z, long *ix, long *iy, long *iz,
    long *jx, long *jy, long *jz, DBL *sx, DBL *sy, DBL *sz, DBL *tx, DBL *ty,
    DBL *tz);
extern DBL Noise(DBL x, DBL y, DBL z);
extern void DNoise(Vector3D *result, DBL x, DBL y, DBL z);
extern DBL cycloidal(DBL value);
extern DBL Triangle_Wave(DBL value);
extern DBL Turbulence(DBL x, DBL y, DBL z, int octaves);
extern void DTurbulence(Vector3D *result, DBL x, DBL y, DBL z, int octaves);
extern void Translate_Texture(Texture **Texture_Ptr, Vector3D *Vector);
extern void Rotate_Texture(Texture **Texture_Ptr, Vector3D *Vector);
extern void Scale_Texture(Texture **Texture_Ptr, Vector3D *Vector);

extern Texture *Copy_Texture(Texture *Texture);
extern Texture *Get_Texture();

#endif

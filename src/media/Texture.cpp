/****************************************************************************
 *                     texture.c
 *
 *  This module implements texturing functions such as noise, turbulence and
 *  texture transformation functions. The actual texture routines are in the
 *  files txtcolor.c, txtbump.c, txtmap.c, etc.
 *  The noise function used here is the one described by Ken Perlin in
 *  "Hypertexture", SIGGRAPH '89 Conference Proceedings page 253.
 *
 *****************************************************************************/
/*
    Some texture ideas garnered from SIGGRAPH '85 Volume 19 Number 3,
    "An Image Synthesizer" By Ken Perlin.
    Further Ideas Garnered from "The RenderMan Companion" (Addison Wesley)
*/

#include <cstdlib>

#include "media/Texture.h"
#include "common/Statistics.h"
#include "common/logger/Logger.h"
#include <cstdio>
#include "common/linealAlgebra/Transformation.h"
#include "common/linealAlgebra/Vector3Dd.h"

static Texture *defaultTextureInstance;
static double *sinTableInstance;
static double frequencyInstance[Texture::NUMBER_OF_WAVES];
static Vector3Dd waveSourcesInstance[Texture::NUMBER_OF_WAVES];
static double *rTableInstance;
static short *hashTableInstance;

Texture *&
TextureUtils::defaultTexture()
{
    return defaultTextureInstance;
}

double *&
TextureUtils::rTable()
{
    return rTableInstance;
}

short *&
TextureUtils::hashTable()
{
    return hashTableInstance;
}

double *&
TextureUtils::sinTable()
{
    return sinTableInstance;
}

double *
TextureUtils::waveFrequency()
{
    return frequencyInstance;
}

Vector3Dd *
TextureUtils::waveSources()
{
    return waveSourcesInstance;
}

static unsigned short crcTableInstance[256] = {0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0,
    0x0280, 0xc241, 0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481,
    0x0440, 0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
    0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841, 0xd801,
    0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40, 0x1e00, 0xdec1,
    0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41, 0x1400, 0xd4c1, 0xd581,
    0x1540, 0xd701, 0x17c0, 0x1680, 0xd641, 0xd201, 0x12c0, 0x1380, 0xd341,
    0x1100, 0xd1c1, 0xd081, 0x1040, 0xf001, 0x30c0, 0x3180, 0xf141, 0x3300,
    0xf3c1, 0xf281, 0x3240, 0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0,
    0x3480, 0xf441, 0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80,
    0xfe41, 0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
    0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41, 0xee01,
    0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40, 0xe401, 0x24c0,
    0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640, 0x2200, 0xe2c1, 0xe381,
    0x2340, 0xe101, 0x21c0, 0x2080, 0xe041, 0xa001, 0x60c0, 0x6180, 0xa141,
    0x6300, 0xa3c1, 0xa281, 0x6240, 0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501,
    0x65c0, 0x6480, 0xa441, 0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0,
    0x6e80, 0xae41, 0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881,
    0x6840, 0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
    0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40, 0xb401,
    0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640, 0x7200, 0xb2c1,
    0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041, 0x5000, 0x90c1, 0x9181,
    0x5140, 0x9301, 0x53c0, 0x5280, 0x9241, 0x9601, 0x56c0, 0x5780, 0x9741,
    0x5500, 0x95c1, 0x9481, 0x5440, 0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00,
    0x9fc1, 0x9e81, 0x5e40, 0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0,
    0x5880, 0x9841, 0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81,
    0x4a40, 0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
    0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641, 0x8201,
    0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040};

unsigned short *
TextureUtils::crcTable()
{
    return crcTableInstance;
}

double
TextureUtils::floorInline(double x)
{
    return (x >= 0.0) ? floor(x) : (0.0 - floor(0.0 - x) - 1.0);
}

double
TextureUtils::fabsInline(double x)
{
    return (x < 0.0) ? (0.0 - x) : x;
}

double
TextureUtils::sCurve(double a)
{
    return a * a * (3.0 - 2.0 * a);
}

short
TextureUtils::hash3d(long a, long b, long c)
{
    return TextureUtils::hashTable()[(
        int)(TextureUtils::hashTable()[(int)(TextureUtils::hashTable()[(int)(a & 0xfffL)] ^ (b & 0xfffL))] ^
             (c & 0xfffL))];
}

double
TextureUtils::incrSum(int m, double s, double x, double y, double z)
{
    return s * (TextureUtils::rTable()[m] * 0.5 + TextureUtils::rTable()[m + 1] * x +
                   TextureUtils::rTable()[m + 2] * y + TextureUtils::rTable()[m + 3] * z);
}

void
TextureUtils::computeColour(
    RGBAColor *colour, RGBAColorPalette *colourMap, double value)
{
    int i;
    RGBAColorPaletteSpan *ent;
    double fraction;

    if (value > 1.0) {
        value = 1.0;
    }

    if (value < 0.0) {
        value = 0.0;
    }

    for (i = 0, ent = &(colourMap->Colour_Map_Entries[0]);
        i < colourMap->numberOfEntries; i++, ent++) {
        if ((value >= ent->start) && (value <= ent->end)) {
            fraction = (value - ent->start) / (ent->end - ent->start);
            colour->Red =
                ent->startColour.Red +
                fraction * (ent->endColour.Red - ent->startColour.Red);
            colour->Green =
                ent->startColour.Green +
                fraction * (ent->endColour.Green - ent->startColour.Green);
            colour->Blue =
                ent->startColour.Blue +
                fraction * (ent->endColour.Blue - ent->startColour.Blue);
            colour->Alpha =
                ent->startColour.Alpha +
                fraction * (ent->endColour.Alpha - ent->startColour.Alpha);
            return;
        }
    }

    colour->Red = 0.0;
    colour->Green = 0.0;
    colour->Blue = 0.0;
    colour->Alpha = 0.0;
    Logger::info("No colour for value: %g\n", value);
}

void
TextureUtils::initializeNoise()
{
    int i = 0;
    Vector3Dd point;

    TextureUtils::InitRTable();

    if ((TextureUtils::sinTable() = new double[Texture::SINTABSIZE]) == nullptr) {
        Logger::info("Cannot allocate memory for sine table\n");
        exit(1);
    }

    for (i = 0; i < Texture::SINTABSIZE; i++) {
        TextureUtils::sinTable()[i] = sin(i / (double)Texture::SINTABSIZE * (3.14159265359 * 2.0));
    }

    for (i = 0; i < Texture::NUMBER_OF_WAVES; i++) {
        TextureUtils::DNoise(&point, (double)i, 0.0, 0.0);
        VectorOps::vNormalize(TextureUtils::waveSources()[i], point);
        TextureUtils::waveFrequency()[i] = (rand() & Texture::RNDMASK) / Texture::rndDivisor + 0.01;
    }
}

void
TextureUtils::InitTextureTable()
{
    int i;
    int j;
    int temp;

    srand(0);

    TextureUtils::hashTable() = new short int[4096];
    if (TextureUtils::hashTable() == nullptr) {
        Logger::info("Cannot allocate memory for hash table\n");
        exit(1);
    }
    for (i = 0; i < 4096; i++) {
        TextureUtils::hashTable()[i] = i;
    }
    for (i = 4095; i >= 0; i--) {
        j = rand() % 4096;
        temp = TextureUtils::hashTable()[i];
        TextureUtils::hashTable()[i] = TextureUtils::hashTable()[j];
        TextureUtils::hashTable()[j] = temp;
    }
}

/* modified by AAC to work properly with little bitty integers (16 bits) */
void
TextureUtils::InitRTable()
{
    int i;
    Vector3Dd rp;

    TextureUtils::InitTextureTable();

    TextureUtils::rTable() = new double[Texture::MAXSIZE];
    if (TextureUtils::rTable() == nullptr) {
        Logger::info("Cannot allocate memory for TextureUtils::rTable()\n");
        exit(1);
    }

    for (i = 0; i < Texture::MAXSIZE; i++) {
        rp.x = rp.y = rp.z = (double)i;
        TextureUtils::rTable()[i] = (unsigned int)TextureUtils::R(&rp) * Texture::realScale - 1.0;
    }
}

int
TextureUtils::R(Vector3Dd *v)
{
    v->x *= .12345;
    v->y *= .12345;
    v->z *= .12345;

    return (TextureUtils::Crc16((char *)v, sizeof(Vector3Dd)));
}

/*
 * Note that passing a Vector3Dd array to Crc16 and interpreting it as
 * an array of chars means that machines with different floating-point
 * representation schemes will evaluate TextureUtils::Noise(point) differently.
 */

int
TextureUtils::Crc16(char *buf, int count)
{
    unsigned short crc = 0;

    while (count--) {
        crc = (crc >> 8) ^ TextureUtils::crcTable()[(unsigned char)(crc ^ *buf++)];
    }

    return ((int)crc);
}

/*
Robert's Skinner's Perlin-style "Noise" function - modified by AAC
to ensure uniformly distributed clamped values between 0 and 1.0...
*/
void
setupLattice(double *x, double *y, double *z, long *ix, long *iy, long *iz,
    long *jx, long *jy, long *jz, double *sx, double *sy, double *sz,
    double *tx, double *ty, double *tz)
{
    /* ensures the values are positive. */
    *x -= Texture::MINX;
    *y -= Texture::MINY;
    *z -= Texture::MINZ;

    /* its equivalent integer lattice point. */
    *ix = (long)*x;
    *iy = (long)*y;
    *iz = (long)*z;
    *jx = *ix + 1;
    *jy = *iy + 1;
    *jz = *iz + 1;

    *sx = TextureUtils::sCurve(*x - *ix);
    *sy = TextureUtils::sCurve(*y - *iy);
    *sz = TextureUtils::sCurve(*z - *iz);

    /* the complement values of sx,sy,sz */
    *tx = 1.0 - *sx;
    *ty = 1.0 - *sy;
    *tz = 1.0 - *sz;
}

double
TextureUtils::Noise(double x, double y, double z)
{
    long ix;
    long iy;
    long iz;
    long jx;
    long jy;
    long jz;
    double sx;
    double sy;
    double sz;
    double tx;
    double ty;
    double tz;
    double sum;
    short m;

    Statistics::global().callsToNoise++;

    setupLattice(
        &x, &y, &z, &ix, &iy, &iz, &jx, &jy, &jz, &sx, &sy, &sz, &tx, &ty, &tz);

    /*
     *  interpolate!
     */
    m = TextureUtils::hash3d(ix, iy, iz) & 0xFF;
    sum = TextureUtils::incrSum(m, (tx * ty * tz), (x - ix), (y - iy), (z - iz));

    m = TextureUtils::hash3d(jx, iy, iz) & 0xFF;
    sum += TextureUtils::incrSum(m, (sx * ty * tz), (x - jx), (y - iy), (z - iz));

    m = TextureUtils::hash3d(ix, jy, iz) & 0xFF;
    sum += TextureUtils::incrSum(m, (tx * sy * tz), (x - ix), (y - jy), (z - iz));

    m = TextureUtils::hash3d(jx, jy, iz) & 0xFF;
    sum += TextureUtils::incrSum(m, (sx * sy * tz), (x - jx), (y - jy), (z - iz));

    m = TextureUtils::hash3d(ix, iy, jz) & 0xFF;
    sum += TextureUtils::incrSum(m, (tx * ty * sz), (x - ix), (y - iy), (z - jz));

    m = TextureUtils::hash3d(jx, iy, jz) & 0xFF;
    sum += TextureUtils::incrSum(m, (sx * ty * sz), (x - jx), (y - iy), (z - jz));

    m = TextureUtils::hash3d(ix, jy, jz) & 0xFF;
    sum += TextureUtils::incrSum(m, (tx * sy * sz), (x - ix), (y - jy), (z - jz));

    m = TextureUtils::hash3d(jx, jy, jz) & 0xFF;
    sum += TextureUtils::incrSum(m, (sx * sy * sz), (x - jx), (y - jy), (z - jz));

    sum = sum + 0.5; /* range at this point -0.5 - 0.5... */

    if (sum < 0.0) {
        sum = 0.0;
    }
    if (sum > 1.0) {
        sum = 1.0;
    }

    return (sum);
}

/*
Vector-valued version of "Noise"
*/
void
TextureUtils::DNoise(Vector3Dd *result, double x, double y, double z)
{
    long ix;
    long iy;
    long iz;
    long jx;
    long jy;
    long jz;
    double px;
    double py;
    double pz;
    double s;
    double sx;
    double sy;
    double sz;
    double tx;
    double ty;
    double tz;
    short m;

    Statistics::global().callsToDNoise++;

    setupLattice(
        &x, &y, &z, &ix, &iy, &iz, &jx, &jy, &jz, &sx, &sy, &sz, &tx, &ty, &tz);

    /*
     *  interpolate!
     */
    m = TextureUtils::hash3d(ix, iy, iz) & 0xFF;
    px = x - ix;
    py = y - iy;
    pz = z - iz;
    s = tx * ty * tz;
    result->x = TextureUtils::incrSum(m, s, px, py, pz);
    result->y = TextureUtils::incrSum(m + 4, s, px, py, pz);
    result->z = TextureUtils::incrSum(m + 8, s, px, py, pz);

    m = TextureUtils::hash3d(jx, iy, iz) & 0xFF;
    px = x - jx;
    s = sx * ty * tz;
    result->x += TextureUtils::incrSum(m, s, px, py, pz);
    result->y += TextureUtils::incrSum(m + 4, s, px, py, pz);
    result->z += TextureUtils::incrSum(m + 8, s, px, py, pz);

    m = TextureUtils::hash3d(jx, jy, iz) & 0xFF;
    py = y - jy;
    s = sx * sy * tz;
    result->x += TextureUtils::incrSum(m, s, px, py, pz);
    result->y += TextureUtils::incrSum(m + 4, s, px, py, pz);
    result->z += TextureUtils::incrSum(m + 8, s, px, py, pz);

    m = TextureUtils::hash3d(ix, jy, iz) & 0xFF;
    px = x - ix;
    s = tx * sy * tz;
    result->x += TextureUtils::incrSum(m, s, px, py, pz);
    result->y += TextureUtils::incrSum(m + 4, s, px, py, pz);
    result->z += TextureUtils::incrSum(m + 8, s, px, py, pz);

    m = TextureUtils::hash3d(ix, jy, jz) & 0xFF;
    pz = z - jz;
    s = tx * sy * sz;
    result->x += TextureUtils::incrSum(m, s, px, py, pz);
    result->y += TextureUtils::incrSum(m + 4, s, px, py, pz);
    result->z += TextureUtils::incrSum(m + 8, s, px, py, pz);

    m = TextureUtils::hash3d(jx, jy, jz) & 0xFF;
    px = x - jx;
    s = sx * sy * sz;
    result->x += TextureUtils::incrSum(m, s, px, py, pz);
    result->y += TextureUtils::incrSum(m + 4, s, px, py, pz);
    result->z += TextureUtils::incrSum(m + 8, s, px, py, pz);

    m = TextureUtils::hash3d(jx, iy, jz) & 0xFF;
    py = y - iy;
    s = sx * ty * sz;
    result->x += TextureUtils::incrSum(m, s, px, py, pz);
    result->y += TextureUtils::incrSum(m + 4, s, px, py, pz);
    result->z += TextureUtils::incrSum(m + 8, s, px, py, pz);

    m = TextureUtils::hash3d(ix, iy, jz) & 0xFF;
    px = x - ix;
    s = tx * ty * sz;
    result->x += TextureUtils::incrSum(m, s, px, py, pz);
    result->y += TextureUtils::incrSum(m + 4, s, px, py, pz);
    result->z += TextureUtils::incrSum(m + 8, s, px, py, pz);
}

double
TextureUtils::Turbulence(double x, double y, double z, int octaves)
{
    int i; /* added -dmf */
    double t = 0.0;
    double scale;
    double value;

    for (i = 0, scale = 1; i < octaves; i++, scale *= 0.5) {
        value = TextureUtils::Noise(x / scale, y / scale, z / scale);
        t += TextureUtils::fabsInline(value) * scale;
    }
    return (t);
}

void
TextureUtils::DTurbulence(
    Vector3Dd *result, double x, double y, double z, int octaves)
{
    int i; /* added -dmf */
    double scale;
    Vector3Dd value;

    result->x = 0.0;
    result->y = 0.0;
    result->z = 0.0;

    value.x = value.y = value.z = 0.0;

    for (i = 0, scale = 1; i < octaves; i++, scale *= 0.5) {
        TextureUtils::DNoise(&value, x / scale, y / scale, z / scale);
        result->x += value.x * scale;
        result->y += value.y * scale;
        result->z += value.z * scale;
    }
}

double
TextureUtils::cycloidal(double value)
{
    int indx;

    if (value >= 0.0) {
        indx = (int)((value - floor(value)) * Texture::SINTABSIZE);
        return (TextureUtils::sinTable()[indx]);
    }

    indx = (int)((0.0 - (value + floor(0.0 - value))) * Texture::SINTABSIZE);
    return (0.0 - TextureUtils::sinTable()[indx]);
}

double
TextureUtils::triangleWave(double value)
{
    double offset;
    double temp1;

    if (value >= 0.0) {
        offset = value - floor(value);
    } else {
        temp1 = -1.0 - floor(fabs(value));
        offset = value - temp1;
    }
    if (offset >= 0.5) {
        return (2.0 * (1.0 - offset));
    }
    return (2.0 * offset);
}

void
TextureUtils::translateTexture(Texture **texturePtr, Vector3Dd *vector)
{
    Texture *texture = *texturePtr;
    Transformation transformation;

    while (texture != nullptr) {
        if (((texture->textureNumber != Texture::NO_TEXTURE) &&
                (texture->textureNumber != Texture::COLOUR_TEXTURE)) ||
            (texture->bumpNumber != Texture::NO_BUMPS)) {

            if (texture->constantFlag) {
                texture = TextureUtils::copyTexture(texture);
                *texturePtr = texture;
                texture->constantFlag = false;
            }

            if (!texture->Texture_Transformation) {
                texture->Texture_Transformation =
                    Transformation::getTransformation();
            }
            Transformation::getTranslationTransformation(
                &transformation, vector);
            Transformation::composeTransformations(
                texture->Texture_Transformation, &transformation);
            if (texture->textureNumber == Texture::CHECKER_TEXTURE_TEXTURE) {
                TextureUtils::translateTexture(
                    (Texture **)&texture->Colour1, vector);
                TextureUtils::translateTexture(
                    (Texture **)&texture->Colour2, vector);
            }
        }
        texturePtr = &texture->Next_Texture;
        texture = texture->Next_Texture;
    }
}

Texture *
TextureUtils::copyTexture(Texture *texture)
{
    Texture *newTexture;
    Texture *localTexture;
    Texture *firstTexture;
    Texture *previousTexture;

    previousTexture = firstTexture = nullptr;

    for (localTexture = texture; localTexture != nullptr;
        localTexture = localTexture->Next_Texture) {
        newTexture = TextureUtils::getTexture();
        *newTexture = *localTexture;

        if (firstTexture == nullptr) {
            firstTexture = newTexture;
        }

        if (previousTexture != nullptr) {
            previousTexture->Next_Texture = newTexture;
        }

        if (newTexture->Texture_Transformation) {
            newTexture->Texture_Transformation = new Transformation;
            if (newTexture->Texture_Transformation == nullptr) {
                Logger::error(
                    "Out of memory. Cannot allocate texture transformation\n");
                exit(1);
            }
            *newTexture->Texture_Transformation =
                *localTexture->Texture_Transformation;
        }
        // Deep copy Colour_Map if present (don't share pointers)
        if (newTexture->Colour_Map != nullptr) {
            RGBAColorPalette *newMap = new RGBAColorPalette();
            if (newMap == nullptr) {
                Logger::error(
                    "Out of memory. Cannot allocate colour map\n");
                exit(1);
            }
            newMap->numberOfEntries = localTexture->Colour_Map->numberOfEntries;
            newMap->transparencyFlag = localTexture->Colour_Map->transparencyFlag;
            newMap->Colour_Map_Entries =
                new RGBAColorPaletteSpan[localTexture->Colour_Map->numberOfEntries];
            if (newMap->Colour_Map_Entries == nullptr) {
                Logger::error(
                    "Out of memory. Cannot allocate colour map entries\n");
                exit(1);
            }
            for (int i = 0; i < localTexture->Colour_Map->numberOfEntries; i++) {
                newMap->Colour_Map_Entries[i] =
                    localTexture->Colour_Map->Colour_Map_Entries[i];
            }
            newTexture->Colour_Map = newMap;
        }
        newTexture->constantFlag = false;
        previousTexture = newTexture;
    }
    return (firstTexture);
}

Texture *
TextureUtils::getTexture()
{
    Texture *newTexture;

    newTexture = new Texture;
    if (newTexture == nullptr) {
        Logger::error("Out of memory. Cannot allocate object");
        exit(1);
    }

    newTexture->Next_Texture = nullptr;
    newTexture->Next_Material = nullptr;
    newTexture->numberOfMaterials = 0;
    newTexture->objectReflection = 0.0;
    newTexture->objectAmbient = 0.1;
    newTexture->objectDiffuse = 0.6;
    newTexture->objectBrilliance = 1.0;
    newTexture->objectSpecular = 0.0;
    newTexture->objectRoughness = 0.05;
    newTexture->objectPhong = 0.0;
    newTexture->objectPhongSize = 40;

    newTexture->textureRandomness = 0.0;
    newTexture->bumpAmount = 0.0;
    newTexture->Phase = 0.0;
    newTexture->Frequency = 1.0;
    newTexture->textureNumber = Texture::NO_TEXTURE;
    newTexture->Texture_Transformation = nullptr;
    newTexture->bumpNumber = Texture::NO_BUMPS;
    newTexture->Turbulence = 0.0;
    newTexture->Colour_Map = nullptr;
    newTexture->onceFlag = false;
    newTexture->metallicFlag = false;
    newTexture->Octaves = 6;  /* dmf, for turbulence functs */
    newTexture->Mortar = 0.2; /* rha, for brick texture */

    newTexture->constantFlag = true;
    newTexture->Colour1 = nullptr;
    newTexture->Colour2 = nullptr;
    *&newTexture->textureGradient = Vector3Dd(0.0, 0.0, 0.0);

    newTexture->objectIndexOfRefraction = 1.0;
    newTexture->objectTransmit = 0.0;
    newTexture->objectRefraction = 0.0;
    return (newTexture);
}

void
TextureUtils::rotateTexture(Texture **texturePtr, Vector3Dd *vector)
{
    Texture *texture = *texturePtr;
    Transformation transformation;

    while (texture != nullptr) {
        if (((texture->textureNumber != Texture::NO_TEXTURE) &&
                (texture->textureNumber != Texture::COLOUR_TEXTURE)) ||
            (texture->bumpNumber != Texture::NO_BUMPS)) {

            if (texture->constantFlag) {
                texture = TextureUtils::copyTexture(texture);
                *texturePtr = texture;
                texture->constantFlag = false;
            }

            if (!texture->Texture_Transformation) {
                texture->Texture_Transformation =
                    Transformation::getTransformation();
            }
            Transformation::getRotationTransformation(&transformation, vector);
            Transformation::composeTransformations(
                texture->Texture_Transformation, &transformation);
            if (texture->textureNumber == Texture::CHECKER_TEXTURE_TEXTURE) {
                TextureUtils::rotateTexture(
                    (Texture **)&texture->Colour1, vector);
                TextureUtils::rotateTexture(
                    (Texture **)&texture->Colour2, vector);
            }
        }
        texturePtr = &texture->Next_Texture;
        texture = texture->Next_Texture;
    }
}

void
TextureUtils::scaleTexture(Texture **texturePtr, Vector3Dd *vector)
{
    Texture *texture = *texturePtr;
    Transformation transformation;

    while (texture != nullptr) {
        if (((texture->textureNumber != Texture::NO_TEXTURE) &&
                (texture->textureNumber != Texture::COLOUR_TEXTURE)) ||
            (texture->bumpNumber != Texture::NO_BUMPS)) {

            if (texture->constantFlag) {
                texture = TextureUtils::copyTexture(texture);
                *texturePtr = texture;
                texture->constantFlag = false;
            }

            if (!texture->Texture_Transformation) {
                texture->Texture_Transformation =
                    Transformation::getTransformation();
            }
            Transformation::getScalingTransformation(&transformation, vector);
            Transformation::composeTransformations(
                texture->Texture_Transformation, &transformation);

            if (texture->textureNumber == Texture::CHECKER_TEXTURE_TEXTURE) {
                TextureUtils::scaleTexture(
                    (Texture **)&texture->Colour1, vector);
                TextureUtils::scaleTexture(
                    (Texture **)&texture->Colour2, vector);
            }
        }
        texturePtr = &texture->Next_Texture;
        texture = texture->Next_Texture;
    }
}

/**
Implements texturing utility functions: noise, turbulence, and texture transforms.
Color, bump, and map texture routines are in colorTextureFixture.cpp,
bumpTextureFixture.cpp, and mapTextureFixture.cpp respectively.

References:
[PERL1985] "An Image Synthesizer" (SIGGRAPH '85, Vol. 19 No. 3, pp. 287-296).
[PERL1989] "Hypertexture" (SIGGRAPH '89, p. 253).
"The RenderMan Companion" (Addison Wesley).
*/

#include <cstdlib>

#include "java/util/ArrayList.txx"
#include "media/solidTexture/Texture.h"
#include "media/solidTexture/SolidTextureColorTextures.h"
#include "media/solidTexture/SolidTextureBumpyTextures.h"
#include "common/statistics/SolidTextureStatistics.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include <cstdio>
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

static Texture *defaultTextureInstance;
static double *sinTableInstance;
static double frequencyInstance[Texture::NUMBER_OF_WAVES];
static Vector3Dd waveSourcesInstance[Texture::NUMBER_OF_WAVES];
static double *rTableInstance;
static short *hashTableInstance;

textureUtils* textureUtils::inst_ = nullptr;

textureUtils::textureUtils(SolidTextureStatistics* stats)
    : solidTextureStats_(stats) {}

void
textureUtils::initialize(SolidTextureStatistics* stats)
{
    static textureUtils inst(stats);
    inst_ = &inst;
}

textureUtils&
textureUtils::instance()
{
    return *inst_;
}

Texture *&
textureUtils::defaultTexture()
{
    return defaultTextureInstance;
}

double *&
textureUtils::rTable()
{
    return rTableInstance;
}

short *&
textureUtils::hashTable()
{
    return hashTableInstance;
}

double *&
textureUtils::sinTable()
{
    return sinTableInstance;
}

double *
textureUtils::waveFrequency()
{
    return frequencyInstance;
}

Vector3Dd *
textureUtils::waveSources()
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
textureUtils::crcTable()
{
    return crcTableInstance;
}

double
textureUtils::floorInline(double x)
{
    return (x >= 0.0) ? floor(x) : (0.0 - floor(0.0 - x) - 1.0);
}

double
textureUtils::fabsInline(double x)
{
    return (x < 0.0) ? (0.0 - x) : x;
}

/** [PERL1985].289 - S-curve interpolation function for smooth lattice transitions. */
double
textureUtils::sCurve(double a)
{
    return a * a * (3.0 - 2.0 * a);
}

/** [PERL1985].289 - Hash function using permutation table for pseudorandom lattice values. */
short
textureUtils::hash3d(long a, long b, long c)
{
    return hashTable()[(
        int)(hashTable()[(int)(hashTable()[(int)(a & 0xfffL)] ^ (b & 0xfffL))] ^
             (c & 0xfffL))];
}

double
textureUtils::incrSum(int m, double s, double x, double y, double z)
{
    return s * (rTable()[m] * 0.5 + rTable()[m + 1] * x +
                   rTable()[m + 2] * y + rTable()[m + 3] * z);
}

void
textureUtils::computeColor(
    ColorRgba *color, RGBAColorPalette *colorMap, double value)
{
    ColorRgba *c = colorMap->evalLinear(value);
    *color = *c;
    delete c;
}

void
textureUtils::initializeNoise()
{
    int i = 0;
    Vector3Dd point;

    InitRTable();

    if ((sinTable() = new double[Texture::SINTABSIZE]) == nullptr) {
        Logger::reportMessage("Texture", Logger::FATAL_ERROR, "", "Cannot allocate memory for sine table\n");
    }

    for (i = 0; i < Texture::SINTABSIZE; i++) {
        sinTable()[i] = sin(i / (double)Texture::SINTABSIZE * (3.14159265359 * 2.0));
    }

    for (i = 0; i < Texture::NUMBER_OF_WAVES; i++) {
        DNoise(&point, (double)i, 0.0, 0.0);
        waveSources()[i] = point.normalizedFast();
        waveFrequency()[i] = (rand() & Texture::RNDMASK) / Texture::rndDivisor + 0.01;
    }
}

/** [PERL1985].289 - Initialize permutation hash table for lattice pseudorandom values. */
void
textureUtils::InitTextureTable()
{
    int i;
    int j;
    int temp;

    srand(0);

    hashTable() = new short int[4096];
    if (hashTable() == nullptr) {
        Logger::reportMessage("Texture", Logger::FATAL_ERROR, "", "Cannot allocate memory for hash table\n");
    }
    for (i = 0; i < 4096; i++) {
        hashTable()[i] = i;
    }
    for (i = 4095; i >= 0; i--) {
        j = rand() % 4096;
        temp = hashTable()[i];
        hashTable()[i] = hashTable()[j];
        hashTable()[j] = temp;
    }
}

/**
[PERL1985].289 - Initialize pseudorandom gradient table via CRC hashing.
Modified by AAC to work properly with 16-bit integers.
*/
void
textureUtils::InitRTable()
{
    int i;
    Vector3Dd rp;

    InitTextureTable();

    rTable() = new double[Texture::MAXSIZE];
    if (rTable() == nullptr) {
        Logger::reportMessage("Texture", Logger::FATAL_ERROR, "", "Cannot allocate memory for rTable()\n");
    }

    for (i = 0; i < Texture::MAXSIZE; i++) {
        rp = Vector3Dd((double)i, (double)i, (double)i);
        rTable()[i] = (unsigned int)R(&rp) * Texture::realScale - 1.0;
    }
}

int
textureUtils::R(Vector3Dd *v)
{
    *v = Vector3Dd(v->x() * .12345, v->y() * .12345, v->z() * .12345);

    return (Crc16((char *)v, sizeof(Vector3Dd)));
}

/**
[PERL1985].289 - CRC-based pseudorandom value generator from 3D points.
Note: passing a Vector3Dd as a char array means machines with different
floating-point representations will produce different Noise() values.
*/
int
textureUtils::Crc16(char *buf, int count)
{
    unsigned short crc = 0;

    while (count--) {
        crc = (crc >> 8) ^ crcTable()[(unsigned char)(crc ^ *buf++)];
    }

    return ((int)crc);
}

/**
[PERL1985].289 - Integer lattice setup: map continuous space to lattice points.
Robert Skinner's Perlin-style Noise function, modified by AAC to ensure
uniformly distributed values clamped between 0.0 and 1.0.
*/
void
setupLattice(double *x, double *y, double *z, long *ix, long *iy, long *iz,
    long *jx, long *jy, long *jz, double *sx, double *sy, double *sz,
    double *tx, double *ty, double *tz)
{
    // ensures the values are positive.
    *x -= Texture::MINX;
    *y -= Texture::MINY;
    *z -= Texture::MINZ;

    // its equivalent integer lattice point.
    *ix = (long)*x;
    *iy = (long)*y;
    *iz = (long)*z;
    *jx = *ix + 1;
    *jy = *iy + 1;
    *jz = *iz + 1;

    *sx = textureUtils::instance().sCurve(*x - *ix);
    *sy = textureUtils::instance().sCurve(*y - *iy);
    *sz = textureUtils::instance().sCurve(*z - *iz);

    // the complement values of sx,sy,sz
    *tx = 1.0 - *sx;
    *ty = 1.0 - *sy;
    *tz = 1.0 - *sz;
}

/**
[PERL1985].289 - Perlin noise function: scalar-valued procedural texture
using lattice-based interpolation with pseudorandom gradients.
*/
double
textureUtils::Noise(double x, double y, double z)
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

    solidTextureStats_->callsToNoise++;

    setupLattice(
        &x, &y, &z, &ix, &iy, &iz, &jx, &jy, &jz, &sx, &sy, &sz, &tx, &ty, &tz);

    // interpolate!
    m = hash3d(ix, iy, iz) & 0xFF;
    sum = incrSum(m, (tx * ty * tz), (x - ix), (y - iy), (z - iz));

    m = hash3d(jx, iy, iz) & 0xFF;
    sum += incrSum(m, (sx * ty * tz), (x - jx), (y - iy), (z - iz));

    m = hash3d(ix, jy, iz) & 0xFF;
    sum += incrSum(m, (tx * sy * tz), (x - ix), (y - jy), (z - iz));

    m = hash3d(jx, jy, iz) & 0xFF;
    sum += incrSum(m, (sx * sy * tz), (x - jx), (y - jy), (z - iz));

    m = hash3d(ix, iy, jz) & 0xFF;
    sum += incrSum(m, (tx * ty * sz), (x - ix), (y - iy), (z - jz));

    m = hash3d(jx, iy, jz) & 0xFF;
    sum += incrSum(m, (sx * ty * sz), (x - jx), (y - iy), (z - jz));

    m = hash3d(ix, jy, jz) & 0xFF;
    sum += incrSum(m, (tx * sy * sz), (x - ix), (y - jy), (z - jz));

    m = hash3d(jx, jy, jz) & 0xFF;
    sum += incrSum(m, (sx * sy * sz), (x - jx), (y - jy), (z - jz));

    sum = sum + 0.5; // range: -0.5 to 0.5 before clamping

    if (sum < 0.0) {
        sum = 0.0;
    }
    if (sum > 1.0) {
        sum = 1.0;
    }

    return (sum);
}

/**
[PERL1985].289 - DNoise: vector-valued differential of the Noise function.
Returns the gradient (directional derivatives) of the noise field.
*/
void
textureUtils::DNoise(Vector3Dd *result, double x, double y, double z)
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

    solidTextureStats_->callsToDNoise++;

    setupLattice(
        &x, &y, &z, &ix, &iy, &iz, &jx, &jy, &jz, &sx, &sy, &sz, &tx, &ty, &tz);

    // interpolate!
    m = hash3d(ix, iy, iz) & 0xFF;
    px = x - ix;
    py = y - iy;
    pz = z - iz;
    s = tx * ty * tz;
    double rx = incrSum(m, s, px, py, pz);
    double ry = incrSum(m + 4, s, px, py, pz);
    double rz = incrSum(m + 8, s, px, py, pz);

    m = hash3d(jx, iy, iz) & 0xFF;
    px = x - jx;
    s = sx * ty * tz;
    rx += incrSum(m, s, px, py, pz);
    ry += incrSum(m + 4, s, px, py, pz);
    rz += incrSum(m + 8, s, px, py, pz);

    m = hash3d(jx, jy, iz) & 0xFF;
    py = y - jy;
    s = sx * sy * tz;
    rx += incrSum(m, s, px, py, pz);
    ry += incrSum(m + 4, s, px, py, pz);
    rz += incrSum(m + 8, s, px, py, pz);

    m = hash3d(ix, jy, iz) & 0xFF;
    px = x - ix;
    s = tx * sy * tz;
    rx += incrSum(m, s, px, py, pz);
    ry += incrSum(m + 4, s, px, py, pz);
    rz += incrSum(m + 8, s, px, py, pz);

    m = hash3d(ix, jy, jz) & 0xFF;
    pz = z - jz;
    s = tx * sy * sz;
    rx += incrSum(m, s, px, py, pz);
    ry += incrSum(m + 4, s, px, py, pz);
    rz += incrSum(m + 8, s, px, py, pz);

    m = hash3d(jx, jy, jz) & 0xFF;
    px = x - jx;
    s = sx * sy * sz;
    rx += incrSum(m, s, px, py, pz);
    ry += incrSum(m + 4, s, px, py, pz);
    rz += incrSum(m + 8, s, px, py, pz);

    m = hash3d(jx, iy, jz) & 0xFF;
    py = y - iy;
    s = sx * ty * sz;
    rx += incrSum(m, s, px, py, pz);
    ry += incrSum(m + 4, s, px, py, pz);
    rz += incrSum(m + 8, s, px, py, pz);

    m = hash3d(ix, iy, jz) & 0xFF;
    px = x - ix;
    s = tx * ty * sz;
    rx += incrSum(m, s, px, py, pz);
    ry += incrSum(m + 4, s, px, py, pz);
    rz += incrSum(m + 8, s, px, py, pz);
    *result = Vector3Dd(rx, ry, rz);
}

/**
[PERL1985].Appendix - Turbulence: functional composition of Noise() over multiple octaves.
Creates self-similar fractal patterns by summing scaled noise at different frequencies.
*/
double
textureUtils::Turbulence(double x, double y, double z, int octaves)
{
    int i; // added -dmf
    double t = 0.0;
    double scale;
    double value;

    for (i = 0, scale = 1; i < octaves; i++, scale *= 0.5) {
        value = Noise(x / scale, y / scale, z / scale);
        t += fabsInline(value) * scale;
    }
    return (t);
}

/**
[PERL1985].Appendix - DTurbulence: vector-valued version of turbulence.
Returns gradient of turbulent field by composing DNoise() over octaves.
*/
void
textureUtils::DTurbulence(
    Vector3Dd *result, double x, double y, double z, int octaves)
{
    int i; // added -dmf
    double scale;
    Vector3Dd value;

    double rx = 0.0;
    double ry = 0.0;
    double rz = 0.0;

    value = Vector3Dd(0.0, 0.0, 0.0);

    for (i = 0, scale = 1; i < octaves; i++, scale *= 0.5) {
        DNoise(&value, x / scale, y / scale, z / scale);
        rx += value.x() * scale;
        ry += value.y() * scale;
        rz += value.z() * scale;
    }
    *result = Vector3Dd(rx, ry, rz);
}

double
textureUtils::cycloidal(double value)
{
    int indx;

    if (value >= 0.0) {
        indx = (int)((value - floor(value)) * Texture::SINTABSIZE);
        return (sinTable()[indx]);
    }

    indx = (int)((0.0 - (value + floor(0.0 - value))) * Texture::SINTABSIZE);
    return (0.0 - sinTable()[indx]);
}

double
textureUtils::triangleWave(double value)
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

static bool
needsTransform(const Texture *texture)
{
    return ((texture->textureNumber != (int)SolidTextureColorTextures::NO_TEXTURE) &&
               (texture->textureNumber != (int)SolidTextureColorTextures::COLOUR_TEXTURE)) ||
           (texture->bumpNumber != (int)SolidTextureBumpyTextures::NO_BUMPS);
}

static void
applyTranslationTransform(Texture *texture, Vector3Dd *vector)
{
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;

    if (!texture->textureTransformation) {
        texture->textureTransformation =
            new Matrix4x4d(Matrix4x4d::identityMatrix());
        texture->textureTransformationInverse =
            new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
    deltaTransformation = Matrix4x4d().translation(
        vector->x(), vector->y(), vector->z()).transpose();
    deltaTransformationInverse = Matrix4x4d().translation(
        0.0 - vector->x(), 0.0 - vector->y(), 0.0 - vector->z()).transpose();
    *texture->textureTransformation =
        texture->textureTransformation->multiply(deltaTransformation);
    *texture->textureTransformationInverse = deltaTransformationInverse.multiply(
        *texture->textureTransformationInverse);
}

void
textureUtils::translateTexture(Texture **texturePtr, Vector3Dd *vector)
{
    Texture *texture = *texturePtr;
    if (texture == nullptr) {
        return;
    }

    if (needsTransform(texture)) {
        if (texture->constantFlag) {
            texture = copyTexture(texture);
            *texturePtr = texture;
        }
        applyTranslationTransform(texture, vector);
        if (texture->textureNumber == (int)SolidTextureColorTextures::CHECKER_TEXTURE_TEXTURE) {
            translateTexture((Texture **)&texture->color1, vector);
            translateTexture((Texture **)&texture->color2, vector);
        }
    }

    for (long int i = 0; i < texture->layers.size(); i++) {
        Texture *layer = texture->layers[i];
        if (needsTransform(layer)) {
            if (layer->constantFlag) {
                layer = copyTexture(layer);
                texture->layers[i] = layer;
            }
            applyTranslationTransform(layer, vector);
            if (layer->textureNumber == (int)SolidTextureColorTextures::CHECKER_TEXTURE_TEXTURE) {
                translateTexture((Texture **)&layer->color1, vector);
                translateTexture((Texture **)&layer->color2, vector);
            }
        }
    }
}

static void
copyTextureNode(Texture *dst, const Texture *src)
{
    if (dst->textureTransformation) {
        dst->textureTransformation =
            new Matrix4x4d(*src->textureTransformation);
        dst->textureTransformationInverse =
            new Matrix4x4d(*src->textureTransformationInverse);
    }
    if (dst->colorMap != nullptr) {
        RGBAColorPalette *newMap = new RGBAColorPalette();
        for (int i = 0; i < src->colorMap->size(); i++) {
            ColorRgba *c = src->colorMap->getColorAt(i);
            if (src->colorMap->hasPositions()) {
                newMap->addColorAt(src->colorMap->getPositionAt(i), *c);
            } else {
                newMap->addColor(*c);
            }
            delete c;
        }
        dst->colorMap = newMap;
    }
    dst->constantFlag = false;
}

Texture *
textureUtils::copyTexture(Texture *texture)
{
    Texture *newHead = getTexture();
    *newHead = *texture;
    copyTextureNode(newHead, texture);

    newHead->layers.clear();
    for (long int i = 0; i < texture->layers.size(); i++) {
        Texture *src = texture->layers[i];
        Texture *copy = getTexture();
        *copy = *src;
        copyTextureNode(copy, src);
        newHead->layers.add(copy);
    }

    return newHead;
}

Texture *
textureUtils::getTexture()
{
    Texture *newTexture;

    newTexture = new Texture;
    if (newTexture == nullptr) {
        Logger::reportMessage("Texture", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate object");
    }

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
    newTexture->phase = 0.0;
    newTexture->frequency = 1.0;
    newTexture->textureNumber = (int)SolidTextureColorTextures::NO_TEXTURE;
    newTexture->textureTransformation = nullptr;
    newTexture->textureTransformationInverse = nullptr;
    newTexture->bumpNumber = (int)SolidTextureBumpyTextures::NO_BUMPS;
    newTexture->turbulence = 0.0;
    newTexture->colorMap = nullptr;
    newTexture->onceFlag = false;
    newTexture->metallicFlag = false;
    newTexture->octaves = 6;  /* dmf, for turbulence functs */
    newTexture->mortar = 0.2; /* rha, for brick texture */

    newTexture->constantFlag = true;
    newTexture->color1 = nullptr;
    newTexture->color2 = nullptr;
    *&newTexture->textureGradient = Vector3Dd(0.0, 0.0, 0.0);

    newTexture->objectIndexOfRefraction = 1.0;
    newTexture->objectTransmit = 0.0;
    newTexture->objectRefraction = 0.0;
    return (newTexture);
}

static void
applyRotationTransform(Texture *texture, Vector3Dd *vector)
{
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;

    if (!texture->textureTransformation) {
        texture->textureTransformation =
            new Matrix4x4d(Matrix4x4d::identityMatrix());
        texture->textureTransformationInverse =
            new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
    deltaTransformation.axisRotationRodrigues(&deltaTransformationInverse, vector);
    *texture->textureTransformation =
        texture->textureTransformation->multiply(deltaTransformation);
    *texture->textureTransformationInverse = deltaTransformationInverse.multiply(
        *texture->textureTransformationInverse);
}

void
textureUtils::rotateTexture(Texture **texturePtr, Vector3Dd *vector)
{
    Texture *texture = *texturePtr;
    if (texture == nullptr) {
        return;
    }

    if (needsTransform(texture)) {
        if (texture->constantFlag) {
            texture = copyTexture(texture);
            *texturePtr = texture;
        }
        applyRotationTransform(texture, vector);
        if (texture->textureNumber == (int)SolidTextureColorTextures::CHECKER_TEXTURE_TEXTURE) {
            rotateTexture((Texture **)&texture->color1, vector);
            rotateTexture((Texture **)&texture->color2, vector);
        }
    }

    for (long int i = 0; i < texture->layers.size(); i++) {
        Texture *layer = texture->layers[i];
        if (needsTransform(layer)) {
            if (layer->constantFlag) {
                layer = copyTexture(layer);
                texture->layers[i] = layer;
            }
            applyRotationTransform(layer, vector);
            if (layer->textureNumber == (int)SolidTextureColorTextures::CHECKER_TEXTURE_TEXTURE) {
                rotateTexture((Texture **)&layer->color1, vector);
                rotateTexture((Texture **)&layer->color2, vector);
            }
        }
    }
}

static void
applyScaleTransform(Texture *texture, Vector3Dd *vector)
{
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;

    if (!texture->textureTransformation) {
        texture->textureTransformation =
            new Matrix4x4d(Matrix4x4d::identityMatrix());
        texture->textureTransformationInverse =
            new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
    deltaTransformation = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    deltaTransformationInverse = Matrix4x4d().scale(
        1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z());
    *texture->textureTransformation =
        texture->textureTransformation->multiply(deltaTransformation);
    *texture->textureTransformationInverse = deltaTransformationInverse.multiply(
        *texture->textureTransformationInverse);
}

void
textureUtils::scaleTexture(Texture **texturePtr, Vector3Dd *vector)
{
    Texture *texture = *texturePtr;
    if (texture == nullptr) {
        return;
    }

    if (needsTransform(texture)) {
        if (texture->constantFlag) {
            texture = copyTexture(texture);
            *texturePtr = texture;
        }
        applyScaleTransform(texture, vector);
        if (texture->textureNumber == (int)SolidTextureColorTextures::CHECKER_TEXTURE_TEXTURE) {
            scaleTexture((Texture **)&texture->color1, vector);
            scaleTexture((Texture **)&texture->color2, vector);
        }
    }

    for (long int i = 0; i < texture->layers.size(); i++) {
        Texture *layer = texture->layers[i];
        if (needsTransform(layer)) {
            if (layer->constantFlag) {
                layer = copyTexture(layer);
                texture->layers[i] = layer;
            }
            applyScaleTransform(layer, vector);
            if (layer->textureNumber == (int)SolidTextureColorTextures::CHECKER_TEXTURE_TEXTURE) {
                scaleTexture((Texture **)&layer->color1, vector);
                scaleTexture((Texture **)&layer->color2, vector);
            }
        }
    }
}

/**
Implements the Perlin-style noise field used to build solid textures: lattice-based
Noise()/DNoise(), their Turbulence()/DTurbulence() fractal compositions, and the
periodic shaping functions (cycloidal, triangleWave) used by wood/marble/agate patterns.

References:
[PERL1985] "An Image Synthesizer" (SIGGRAPH '85, Vol. 19 No. 3, pp. 287-296).
[PERL1989] "Hypertexture" (SIGGRAPH '89, p. 253).
"The RenderMan Companion" (Addison Wesley).
*/

#include "vsdk/toolkit/common/logging/Logger.h"
#include "solidTexture/procedural/ProceduralNoise.h"

double
ProceduralNoise::fabsInline(double x)
{
    return (x < 0.0) ? (0.0 - x) : x;
}

static const unsigned short crcTableData[256] = {0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0,
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

ProceduralNoise::ProceduralNoise(SolidTextureStatistics *solidTextureStatistics)
    : permutationTable(nullptr), rTable(nullptr), sinTable(nullptr),
      solidTextureStatistics(solidTextureStatistics)
{
}

ProceduralNoise::~ProceduralNoise()
{
    delete[] permutationTable;
    delete[] rTable;
    delete[] sinTable;
}

/**
Permutation table built by initialize(); exposed read-only for other hashing uses.
*/
const short *
ProceduralNoise::hashTable() const
{
    return permutationTable;
}

/**
CRC-16 lookup table used internally by initialize(); exposed for other hashing uses.
*/
const unsigned short *
ProceduralNoise::crcTable() const
{
    return crcTableData;
}

/**
[PERL1985].289 - S-curve interpolation function for smooth lattice transitions.
*/
double
ProceduralNoise::sCurve(double a) const
{
    return a * a * (3.0 - 2.0 * a);
}

short
ProceduralNoise::hash3d(long a, long b, long c) const
{
    return permutationTable[(int)(permutationTable[(int)(permutationTable[(int)(a & 0xfffL)] ^ (b & 0xfffL))] ^
                             (c & 0xfffL))];
}

double
ProceduralNoise::incrSum(int m, double s, double x, double y, double z) const
{
    return s * (rTable[m] * 0.5 + rTable[m + 1] * x +
                    rTable[m + 2] * y + rTable[m + 3] * z);
}

/**
[PERL1985].289 - Initialize permutation hash table for lattice pseudorandom values.
*/
void
ProceduralNoise::initTextureTable()
{
    int i;
    int j;
    int temp;

    srand(0);

    permutationTable = new short int[4096];
    if (permutationTable == nullptr) {
        Logger::reportMessage("ProceduralNoise", Logger::FATAL_ERROR, "", "Cannot allocate memory for hash table\n");
    }
    for (i = 0; i < 4096; i++) {
        permutationTable[i] = i;
    }
    for (i = 4095; i >= 0; i--) {
        j = rand() % 4096;
        temp = permutationTable[i];
        permutationTable[i] = permutationTable[j];
        permutationTable[j] = temp;
    }
}

int
ProceduralNoise::crc16(const char *buf, int count) const
{
    unsigned short crc = 0;

    while (count--) {
        crc = (crc >> 8) ^ crcTableData[(unsigned char)(crc ^ *buf++)];
    }

    return ((int)crc);
}

int
ProceduralNoise::r(Vector3Dd *v) const
{
    *v = Vector3Dd(v->x() * .12345, v->y() * .12345, v->z() * .12345);

    return (crc16((char *)v, sizeof(Vector3Dd)));
}

/**
[PERL1985].289 - Initialize pseudorandom gradient table via CRC hashing.
Modified by AAC to work properly with 16-bit integers.
*/
void
ProceduralNoise::initRTable()
{
    int i;
    Vector3Dd rp;

    initTextureTable();

    rTable = new double[MAXSIZE];
    if (rTable == nullptr) {
        Logger::reportMessage("ProceduralNoise", Logger::FATAL_ERROR, "", "Cannot allocate memory for rTable\n");
    }

    for (i = 0; i < MAXSIZE; i++) {
        rp = Vector3Dd((double)i, (double)i, (double)i);
        rTable[i] = (unsigned int)r(&rp) * REAL_SCALE - 1.0;
    }
}

/**
Builds the permutation, gradient, and sine tables used by the functions below.
*/
void
ProceduralNoise::initialize()
{
    initRTable();

    sinTable = new double[SINTAB_SIZE];
    if (sinTable == nullptr) {
        Logger::reportMessage("ProceduralNoise", Logger::FATAL_ERROR, "", "Cannot allocate memory for sine table\n");
    }

    for (int i = 0; i < SINTAB_SIZE; i++) {
        sinTable[i] = sin(i / (double)SINTAB_SIZE * (3.14159265359 * 2.0));
    }
}

/**
[PERL1985].289 - Integer lattice setup: map continuous space to lattice points.
Robert Skinner's Perlin-style Noise function, modified by AAC to ensure
uniformly distributed values clamped between 0.0 and 1.0.
*/
void
ProceduralNoise::setupLattice(double *x, double *y, double *z, long *ix, long *iy, long *iz,
    long *jx, long *jy, long *jz, double *sx, double *sy, double *sz,
    double *tx, double *ty, double *tz) const
{
    // ensures the values are positive.
    *x -= MINX;
    *y -= MINY;
    *z -= MINZ;

    // its equivalent integer lattice point.
    *ix = (long)*x;
    *iy = (long)*y;
    *iz = (long)*z;
    *jx = *ix + 1;
    *jy = *iy + 1;
    *jz = *iz + 1;

    *sx = sCurve(*x - *ix);
    *sy = sCurve(*y - *iy);
    *sz = sCurve(*z - *iz);

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
ProceduralNoise::noise(double x, double y, double z)
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

    if (solidTextureStatistics != nullptr) {
        solidTextureStatistics->callsToNoise++;
    }

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
ProceduralNoise::dNoise(Vector3Dd *result, double x, double y, double z)
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

    if (solidTextureStatistics != nullptr) {
        solidTextureStatistics->callsToDNoise++;
    }

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
ProceduralNoise::turbulence(double x, double y, double z, int octaves)
{
    int i; // added -dmf
    double t = 0.0;
    double scale;
    double value;

    for (i = 0, scale = 1; i < octaves; i++, scale *= 0.5) {
        value = noise(x / scale, y / scale, z / scale);
        t += fabsInline(value) * scale;
    }
    return (t);
}

/**
[PERL1985].Appendix - DTurbulence: vector-valued version of turbulence.
Returns gradient of turbulent field by composing DNoise() over octaves.
*/
void
ProceduralNoise::dTurbulence(
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
        dNoise(&value, x / scale, y / scale, z / scale);
        rx += value.x() * scale;
        ry += value.y() * scale;
        rz += value.z() * scale;
    }
    *result = Vector3Dd(rx, ry, rz);
}

/**
Periodic wave function used to turn noise/turbulence values into band patterns.
*/
double
ProceduralNoise::cycloidal(double value) const
{
    int indx;

    if (value >= 0.0) {
        indx = (int)((value - floor(value)) * SINTAB_SIZE);
        return (sinTable[indx]);
    }

    indx = (int)((0.0 - (value + floor(0.0 - value))) * SINTAB_SIZE);
    return (0.0 - sinTable[indx]);
}

/**
Periodic wave function used to turn noise/turbulence values into band patterns.
*/
double
ProceduralNoise::triangleWave(double value) const
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

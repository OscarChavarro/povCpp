/**
Implements the Perlin-style noise field used to build solid textures: lattice-based
Noise()/DNoise(), their Turbulence()/DTurbulence() fractal compositions, and the
periodic shaping functions (cycloidal, triangleWave) used by wood/marble/agate patterns.

References:
[PERL1985] "An Image Synthesizer" (SIGGRAPH '85, Vol. 19 No. 3, pp. 287-296).
[PERL1989] "Hypertexture" (SIGGRAPH '89, p. 253).
"The RenderMan Companion" (Addison Wesley).
*/

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "solidTexture/procedural/ProceduralNoise.h"

double
ProceduralNoise::fabsInline(double x)
{
    return (x < 0.0) ? (0.0 - x) : x;
}

ProceduralNoise::ProceduralNoise(SolidTextureStatistics *solidTextureStatistics)
    : permutationTable(nullptr), rTable(nullptr),
      sineLookUpTable(11),
      solidTextureStatistics(solidTextureStatistics)
{
}

ProceduralNoise::~ProceduralNoise()
{
    delete[] permutationTable;
    delete[] rTable;
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
CRC-16 lookup table used internally by r(); exposed for other hashing uses.
*/
const LookUpTableChecksum16 &
ProceduralNoise::checksumTable() const
{
    return checksumLookUpTable;
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
ProceduralNoise::r(Vector3Dd *v) const
{
    *v = Vector3Dd(v->x() * .12345, v->y() * .12345, v->z() * .12345);

    return (checksumLookUpTable.eval((char *)v, sizeof(Vector3Dd)));
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
Builds the permutation and gradient tables used by the functions below.
The sine table is built independently by sineLookUpTable's constructor.
*/
void
ProceduralNoise::initialize()
{
    initRTable();
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
    // Ensures the values are positive.
    *x -= MIN_X;
    *y -= MIN_Y;
    *z -= MIN_Z;

    // Its equivalent integer lattice point.
    *ix = (long)*x;
    *iy = (long)*y;
    *iz = (long)*z;
    *jx = *ix + 1;
    *jy = *iy + 1;
    *jz = *iz + 1;

    *sx = sCurve(*x - *ix);
    *sy = sCurve(*y - *iy);
    *sz = sCurve(*z - *iz);

    // The complement values of sx,sy,sz
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

    // Interpolate!
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
ProceduralNoise::differentialNoise(Vector3Dd *result, double x, double y, double z)
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
ProceduralNoise::differentialTurbulence(
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
        differentialNoise(&value, x / scale, y / scale, z / scale);
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
    if (value >= 0.0) {
        return sineLookUpTable.eval(value - java::Math::floor(value));
    }

    return (0.0 - sineLookUpTable.eval(0.0 - (value + java::Math::floor(0.0 - value))));
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
        offset = value - java::Math::floor(value);
    } else {
        temp1 = -1.0 - java::Math::floor(java::Math::abs(value));
        offset = value - temp1;
    }
    if (offset >= 0.5) {
        return (2.0 * (1.0 - offset));
    }
    return (2.0 * offset);
}

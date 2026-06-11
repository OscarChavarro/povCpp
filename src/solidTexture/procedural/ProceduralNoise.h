#ifndef __PROCEDURAL_NOISE_H__
#define __PROCEDURAL_NOISE_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "common/statistics/SolidTextureStatistics.h"

/**
[PERL1985] - Ken Perlin, "An Image Synthesizer", SIGGRAPH '85.
Noise field for solid texturing: lattice-based pseudorandom Noise()
and DNoise() primitives, their fractal Turbulence()/DTurbulence() compositions, and
the periodic shaping functions (cycloidal, triangleWave) used to build patterns such
as wood, marble, and agate on top of them.
*/
class ProceduralNoise {
  private:
    short *permutationTable;
    double *rTable;
    double *sinTable;
    SolidTextureStatistics *solidTextureStatistics;

    static double fabsInline(double x);
    void initTextureTable();
    void initRTable();
    int r(Vector3Dd *v) const;
    int crc16(const char *buf, int count) const;
    short hash3d(long a, long b, long c) const;
    double incrSum(int m, double s, double x, double y, double z) const;
    void setupLattice(double *x, double *y, double *z, long *ix, long *iy, long *iz,
        long *jx, long *jy, long *jz, double *sx, double *sy, double *sz,
        double *tx, double *ty, double *tz) const;

  public:
    static constexpr int MINX = -10000; // Ridiculously large scaling offset to ensure positive lattice coords
    static constexpr int MINY = MINX;
    static constexpr int MINZ = MINX;
    static constexpr int MAXSIZE = 267;
    static constexpr long RNDMASK = 0x7FFF;
    static constexpr float RND_DIVISOR = static_cast<float>(RNDMASK);
    static constexpr int SINTAB_SIZE = 1000;
    static constexpr double REAL_SCALE = (2.0 / 65535.0);

    ProceduralNoise(SolidTextureStatistics *solidTextureStatistics = nullptr);
    ~ProceduralNoise();

    void initialize();
    double sCurve(double a) const;
    double cycloidal(double value) const;
    double triangleWave(double value) const;
    double noise(double x, double y, double z);
    void dNoise(Vector3Dd *result, double x, double y, double z);
    double turbulence(double x, double y, double z, int octaves);
    void dTurbulence(Vector3Dd *result, double x, double y, double z, int octaves);
    const short *hashTable() const;
    const unsigned short *crcTable() const;
};

#endif

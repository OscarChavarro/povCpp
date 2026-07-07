#ifndef __PROCEDURALNOISE__
#define __PROCEDURALNOISE__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/numericalAnalysis/lookUpTables/LookUpTableChecksum16.h"
#include "vsdk/toolkit/numericalAnalysis/lookUpTables/LookUpTableSine.h"
#include "vsdk/toolkit/common/statistics/SolidTextureStatistics.h"
/**
[PERL1985] - Ken Perlin, "An Image Synthesizer", SIGGRAPH '85.
Noise field for solid texturing: lattice-based pseudorandom noise()
and differentialNoise() primitives, their fractal turbulence()/differentialTurbulence()
compositions, and the periodic shaping functions (cycloidal, triangleWave) used to
build patterns such as wood, marble, and agate on top of them.
*/
class ProceduralNoise {
  private:
    static constexpr int MIN_X = -10000; // Ridiculously large scaling offset to ensure positive lattice coords
    static constexpr int MIN_Y = MIN_X;
    static constexpr int MIN_Z = MIN_X;
    static constexpr int MAXSIZE = 267;
    static constexpr double REAL_SCALE = (2.0 / 65535.0);

    short *permutationTable;
    double *rTable;
    LookUpTableSine sineLookUpTable;
    LookUpTableChecksum16 checksumLookUpTable;
    SolidTextureStatistics * const solidTextureStatistics;

    static double fabsInline(double x);
    void initTextureTable();
    void initRTable();
    int r(Vector3Dd *v) const;
    short hash3d(long a, long b, long c) const;
    double incrSum(int m, double s, double x, double y, double z) const;
    void setupLattice(double *x, double *y, double *z, long *ix, long *iy, long *iz,
        long *jx, long *jy, long *jz, double *sx, double *sy, double *sz,
        double *tx, double *ty, double *tz) const;

  public:
    ProceduralNoise(SolidTextureStatistics *solidTextureStatistics = nullptr);
    ~ProceduralNoise();

    void initialize();
    double sCurve(double a) const;
    double cycloidal(double value) const;
    double triangleWave(double value) const;
    double noise(double x, double y, double z) const;
    void differentialNoise(Vector3Dd *result, double x, double y, double z) const;
    double turbulence(double x, double y, double z, int octaves) const;
    void differentialTurbulence(Vector3Dd *result, double x, double y, double z, int octaves) const;
    const short *hashTable() const;
    const LookUpTableChecksum16 &checksumTable() const;
};

#endif

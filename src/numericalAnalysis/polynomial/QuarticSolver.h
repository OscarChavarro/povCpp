#ifndef __QUARTIC_SOLVER__
#define __QUARTIC_SOLVER__

class QuarticSolver {
  private:
    static constexpr double COEFFICIENT_LIMIT = 1.0e-20;
    static constexpr double FUDGE_FACTOR1 = 1.0e11;
    static constexpr double FUDGE_FACTOR2 = -1.0e-5;
    static constexpr double FUDGE_FACTOR3 = 1.0e-7;
    static int hasDifficultCoefficients(int n, const double *coefficients);

  public:
    static int solve(
        const double *coefficients, double *roots, double minValue, double epsilon);
};

#endif

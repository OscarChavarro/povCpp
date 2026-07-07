#ifndef __QUADRATICSOLVER__
#define __QUADRATICSOLVER__

class QuadraticSolver {
  private:
    static constexpr double COEFFICIENT_LIMIT = 1.0e-20;

  public:
    static int solve(const double *coefficients, double *roots);
};

#endif

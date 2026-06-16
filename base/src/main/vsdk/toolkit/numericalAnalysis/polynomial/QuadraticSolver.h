#ifndef __QUADRATIC_SOLVER__
#define __QUADRATIC_SOLVER__

class QuadraticSolver {
  private:
    static constexpr double COEFFICIENT_LIMIT = 1.0e-20;

  public:
    static int solve(const double *coefficients, double *roots);
};

#endif

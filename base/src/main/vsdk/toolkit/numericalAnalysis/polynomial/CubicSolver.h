#ifndef __CUBICSOLVER__
#define __CUBICSOLVER__

class CubicSolver {
  private:
    static constexpr double TWO_PI_3 = 2.0943951023931954923084;
    static constexpr double TWO_PI_43 = 4.1887902047863909846168;
    static double absoluteValue(double x);

  public:
    static int solve(const double *coefficients, double *roots);
};

#endif

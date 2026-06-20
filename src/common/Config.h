#ifndef __CONFIG__
#define __CONFIG__

class Config {
  public:
    static constexpr double SMALL_TOLERANCE = 0.001;
    static constexpr double MAX_DISTANCE = 1.0e7;
    static constexpr double INTERSECTION_EPSILON = 1.0e-5;
    static constexpr double POLYNOMIAL_SOLVER_EPSILON = 1.0e-10;
    static constexpr double PARAMETRIC_CURVE_EPSILON = 1.0e-10;
};

#endif

#ifndef __GEOMETRY_CONFIG__
#define __GEOMETRY_CONFIG__

#include <cstdio>

#include "vsdk/toolkit/common/logging/Logger.h"

class GeometryConfig {
  public:
    static constexpr double SMALL_TOLERANCE = 0.001;
    static constexpr double MAX_DISTANCE = 1.0e7;
    static constexpr double INTERSECTION_EPSILON = 1.0e-5;
    static constexpr double POLYNOMIAL_SOLVER_EPSILON = 1.0e-10;
    static constexpr double PARAMETRIC_CURVE_EPSILON = 1.0e-10;

    static void print();
};

inline void
GeometryConfig::print()
{
    char buffer[256];

    Logger::reportMessage("GeometryConfig", Logger::WARNING, "", "\nGeometryConfig:");

    snprintf(buffer, sizeof(buffer), "    SMALL_TOLERANCE = %.17g", SMALL_TOLERANCE);
    Logger::reportMessage("GeometryConfig", Logger::WARNING, "", buffer);

    snprintf(buffer, sizeof(buffer), "    MAX_DISTANCE = %.17g", MAX_DISTANCE);
    Logger::reportMessage("GeometryConfig", Logger::WARNING, "", buffer);

    snprintf(buffer, sizeof(buffer), "    INTERSECTION_EPSILON = %.17g", INTERSECTION_EPSILON);
    Logger::reportMessage("GeometryConfig", Logger::WARNING, "", buffer);

    snprintf(buffer, sizeof(buffer), "    POLYNOMIAL_SOLVER_EPSILON = %.17g", POLYNOMIAL_SOLVER_EPSILON);
    Logger::reportMessage("GeometryConfig", Logger::WARNING, "", buffer);

    snprintf(buffer, sizeof(buffer), "    PARAMETRIC_CURVE_EPSILON = %.17g", PARAMETRIC_CURVE_EPSILON);
    Logger::reportMessage("GeometryConfig", Logger::WARNING, "", buffer);
}

#endif

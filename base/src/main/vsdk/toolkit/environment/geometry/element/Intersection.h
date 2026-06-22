#ifndef __VSDK_TOOLKIT_ENVIRONMENT_GEOMETRY_ELEMENTS_INTERSECTION_H__
#define __VSDK_TOOLKIT_ENVIRONMENT_GEOMETRY_ELEMENTS_INTERSECTION_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
class Intersection
{
public:
    double t;
    Vector3Dd point;
    Vector3Dd normal;

    inline Intersection() : t(0.0), point(0, 0, 0), normal(0, 0, 0) {}
    inline Intersection(double t, const Vector3Dd& point, const Vector3Dd& normal)
        : t(t), point(point), normal(normal) {}
};

#endif

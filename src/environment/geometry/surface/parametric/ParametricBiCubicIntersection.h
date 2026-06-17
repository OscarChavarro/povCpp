#ifndef __PARAMETRIC_BICUBIC_INTERSECTION_H__
#define __PARAMETRIC_BICUBIC_INTERSECTION_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/element/RayWithSegments.h"

class ParametricBiCubicIntersection {
  public:
    static int intersectSubpatch(int patchType, const RayWithSegments *ray,
        const Vector3Dd *v1, const Vector3Dd *v2, const Vector3Dd *v3,
        const Vector3Dd *n, double d, const Vector3Dd *n1,
        const Vector3Dd *n2, const Vector3Dd *n3, double *depth,
        Vector3Dd *ip, Vector3Dd *ipNorm);
    static int subpatchNormal(const Vector3Dd *v1, const Vector3Dd *v2,
        const Vector3Dd *v3, Vector3Dd *result, double *d);
    static int sphericalBoundsCheck(
        const RayWithSegments *ray, const Vector3Dd *center, double radius);
    static double pointPlaneDistance(
        const Vector3Dd *p, const Vector3Dd *n, const double *d);
};

#endif

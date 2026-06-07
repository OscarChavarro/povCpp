#ifndef __PARAMETRIC_BICUBIC_INTERSECTION_H__
#define __PARAMETRIC_BICUBIC_INTERSECTION_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/elements/RayWithSegments.h"

class ParametricBiCubicPatch;

class ParametricBiCubicIntersection {
  public:
    static int intersectSubpatch(int patchType, RayWithSegments *ray,
        Vector3Dd *v1, Vector3Dd *v2, Vector3Dd *v3, Vector3Dd *n, double d,
        Vector3Dd *n1, Vector3Dd *n2, Vector3Dd *n3, double *depth,
        Vector3Dd *ip, Vector3Dd *ipNorm);
    static int subpatchNormal(Vector3Dd *v1, Vector3Dd *v2, Vector3Dd *v3,
        Vector3Dd *result, double *d);
    static int sphericalBoundsCheck(
        RayWithSegments *ray, Vector3Dd *center, double radius);
    static double pointPlaneDistance(Vector3Dd *p, Vector3Dd *n, double *d);
};

#endif

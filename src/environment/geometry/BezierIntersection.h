#ifndef __BEZIER_INTERSECTION_H__
#define __BEZIER_INTERSECTION_H__

#include "environment/geometry/elements/Ray.h"
#include "common/linealAlgebra/Vector3Dd.h"

class BicubicPatch;

class BezierIntersection {
  public:
    static int intersectSubpatch(int patchType, Ray *ray, Vector3Dd *v1, Vector3Dd *v2,
        Vector3Dd *v3, Vector3Dd *n, double d, Vector3Dd *n1, Vector3Dd *n2, Vector3Dd *n3,
        double *depth, Vector3Dd *ip, Vector3Dd *ipNorm);
    static int subpatchNormal(
        Vector3Dd *v1, Vector3Dd *v2, Vector3Dd *v3, Vector3Dd *result, double *d);
    static int sphericalBoundsCheck(Ray *ray, Vector3Dd *center, double radius);
    static double pointPlaneDistance(Vector3Dd *p, Vector3Dd *n, double *d);
};

#endif

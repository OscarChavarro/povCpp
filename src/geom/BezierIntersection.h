#ifndef __BEZIER_INTERSECTION_H__
#define __BEZIER_INTERSECTION_H__

#include "common/Ray.h"
#include "common/Vector3D.h"

class BicubicPatch;

class BezierIntersection {
  public:
    static int intersectSubpatch(int patchType, Ray *ray, Vector3D *v1, Vector3D *v2,
        Vector3D *v3, Vector3D *n, double d, Vector3D *n1, Vector3D *n2, Vector3D *n3,
        double *depth, Vector3D *ip, Vector3D *ipNorm);
    static int subpatchNormal(
        Vector3D *v1, Vector3D *v2, Vector3D *v3, Vector3D *result, double *d);
    static int sphericalBoundsCheck(Ray *ray, Vector3D *center, double radius);
    static double pointPlaneDistance(Vector3D *p, Vector3D *n, double *d);
};

#endif

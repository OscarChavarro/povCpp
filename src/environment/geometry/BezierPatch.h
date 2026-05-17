#ifndef __BEZIER_PATCH_H__
#define __BEZIER_PATCH_H__

#include "environment/geometry/elements/Ray.h"
#include "environment/geometry/GeometryOperations.h"

class BicubicPatch;
class PriorityQueueNode;

class BezierPatch {
  public:
    static int intersectBicubicPatch0(Ray *ray, BicubicPatch *shape, double *depths);
    static int intersectBicubicPatch1(Ray *ray, BicubicPatch *shape, double *depths);
    static int intersectBicubicPatch2(Ray *ray, BicubicPatch *shape, double *depths);
    static int intersectBicubicPatch3(Ray *ray, BicubicPatch *shape, double *depths);
    static int intersectBicubicPatch4(Ray *ray, BicubicPatch *shape, double *depths);
    static int allBicubicPatchIntersections(
        SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue);
};

#endif

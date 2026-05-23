#ifndef __INFINITE_PLANE_H__
#define __INFINITE_PLANE_H__

#include "common/LegacyBoolean.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

class InfinitePlane : public Geometry {
  public:
    Vector3Dd normalVector;
    double Distance;
    double VPNormDotOrigin;
    int VPCached;

    static int allPlaneIntersections(SimpleBody *object, RayWithSegments *ray,
        PriorityQueueNode *depthQueue);
    static int intersectPlane(
        RayWithSegments *ray, InfinitePlane *plane, double *depth);
    static int insidePlane(Vector3Dd *point, SimpleBody *object);
    static void planeNormal(
        Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint);
    static void *copyPlane(SimpleBody *object);
    static void translatePlane(SimpleBody *object, Vector3Dd *vector);
    static void rotatePlane(SimpleBody *object, Vector3Dd *vector);
    static void scalePlane(SimpleBody *object, Vector3Dd *vector);
    static void invertPlane(SimpleBody *object);
};

extern Methods planeMethods;
extern InfinitePlane *getPlaneShape(void);

#endif

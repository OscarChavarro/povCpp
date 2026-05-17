#ifndef __INFINITE_PLANE_H__
#define __INFINITE_PLANE_H__

#include "common/FrameConfig.h"
#include "app/PovApp.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "geom/GeometryOperations.h"

class InfinitePlane : public Geometry {
  public:
    Vector3Dd Normal_Vector;
    double Distance;
    double VPNormDotOrigin;
    int VPCached;

    static int allPlaneIntersections(
        SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue);
    static int intersectPlane(Ray *ray, InfinitePlane *plane, double *depth);
    static int insidePlane(Vector3Dd *point, SimpleBody *object);
    static void planeNormal(
        Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint);
    static void *copyPlane(SimpleBody *object);
    static void translatePlane(SimpleBody *object, Vector3Dd *vector);
    static void rotatePlane(SimpleBody *object, Vector3Dd *vector);
    static void scalePlane(SimpleBody *object, Vector3Dd *vector);
    static void invertPlane(SimpleBody *object);
};

extern Methods Plane_Methods;
extern InfinitePlane *getPlaneShape(void);

#endif

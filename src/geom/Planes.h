#ifndef __PLANES_H__
#define __PLANES_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Geometry.h"

class InfinitePlane : public Geometry {
  public:
    Vector3D Normal_Vector;
    double Distance;
    double VPNormDotOrigin;
    int VPCached;

    static int allPlaneIntersections(
        SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue);
    static int intersectPlane(Ray *ray, InfinitePlane *plane, double *depth);
    static int insidePlane(Vector3D *point, SimpleBody *object);
    static void planeNormal(
        Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint);
    static void *copyPlane(SimpleBody *object);
    static void translatePlane(SimpleBody *object, Vector3D *vector);
    static void rotatePlane(SimpleBody *object, Vector3D *vector);
    static void scalePlane(SimpleBody *object, Vector3D *vector);
    static void invertPlane(SimpleBody *object);
};

extern Methods Plane_Methods;
extern InfinitePlane *getPlaneShape(void);

#endif

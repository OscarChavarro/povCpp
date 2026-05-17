#ifndef __SPHERES_H__
#define __SPHERES_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Geometry.h"

class Sphere : public Geometry {
  public:
    Vector3D Center;
    DBL Radius;
    DBL Radius_Squared;
    DBL Inverse_Radius;
    Vector3D VPOtoC;
    DBL VPOCSquared;
    short VPinside, VPCached, Inverted;

    static int allSphereIntersections(
        SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue);
    static int intersectSphere(Ray *ray, Sphere *sphere, DBL *depth1, DBL *depth2);
    static int insideSphere(Vector3D *point, SimpleBody *object);
    static void sphereNormal(
        Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint);
    static void *copySphere(SimpleBody *object);
    static void translateSphere(SimpleBody *object, Vector3D *vector);
    static void rotateSphere(SimpleBody *object, Vector3D *vector);
    static void scaleSphere(SimpleBody *object, Vector3D *vector);
    static void invertSphere(SimpleBody *object);
};

extern Methods Sphere_Methods;
extern Sphere *getSphereShape(void);

#endif

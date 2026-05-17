#ifndef __SPHERE_H__
#define __SPHERE_H__

#include "common/FrameConfig.h"
#include "app/PovApp.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

class Sphere : public Geometry {
  public:
    Vector3Dd Center;
    double Radius;
    double Radius_Squared;
    double Inverse_Radius;
    Vector3Dd VPOtoC;
    double VPOCSquared;
    short VPinside, VPCached, Inverted;

    static int allSphereIntersections(
        SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue);
    static int intersectSphere(Ray *ray, Sphere *sphere, double *depth1, double *depth2);
    static int insideSphere(Vector3Dd *point, SimpleBody *object);
    static void sphereNormal(
        Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint);
    static void *copySphere(SimpleBody *object);
    static void translateSphere(SimpleBody *object, Vector3Dd *vector);
    static void rotateSphere(SimpleBody *object, Vector3Dd *vector);
    static void scaleSphere(SimpleBody *object, Vector3Dd *vector);
    static void invertSphere(SimpleBody *object);
};

extern Methods Sphere_Methods;
extern Sphere *getSphereShape(void);

#endif

#ifndef __SPHERE_H__
#define __SPHERE_H__

#include "common/FrameConfig.h"
#include "app/PovApp.h"
#include "common/Vector3D.h"
#include "geom/GeometryOperations.h"

class Sphere : public Geometry {
  public:
    Vector3D Center;
    double Radius;
    double Radius_Squared;
    double Inverse_Radius;
    Vector3D VPOtoC;
    double VPOCSquared;
    short VPinside, VPCached, Inverted;

    static int allSphereIntersections(
        SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue);
    static int intersectSphere(Ray *ray, Sphere *sphere, double *depth1, double *depth2);
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

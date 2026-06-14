#ifndef __SPHERE_H__
#define __SPHERE_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

class Sphere : public Geometry {
  public:
    static Methods methodTable;
    Vector3Dd Center;
    double Radius;
    double radiusSquared;
    double inverseRadius;
    Vector3Dd VPOtoC;
    double VPOCSquared;
    short VPinside;
    bool VPCached;
    bool Inverted;

    static int allSphereIntersections(SimpleBody *object, RayWithSegments *ray,
        java::PriorityQueue<Intersection> *depthQueue);
    static int intersectSphere(const RayWithSegments *ray, Sphere *sphere,
        double *depth1, double *depth2);
    static int insideSphere(Vector3Dd *point, SimpleBody *object);
    static void sphereNormal(
        Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint);
    static void *copySphere(SimpleBody *object);
    static void translateSphere(SimpleBody *object, Vector3Dd *vector);
    static void rotateSphere(SimpleBody *object, Vector3Dd *vector);
    static void scaleSphere(SimpleBody *object, Vector3Dd *vector);
    static void invertSphere(SimpleBody *object);
};

#endif

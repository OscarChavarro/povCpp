#ifndef __TRIANGLE_H__
#define __TRIANGLE_H__

#include "common/LegacyBoolean.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

class SmoothTriangle;

class Triangle : public Geometry {
  public:
    static Methods methodTable;
    static Methods smoothMethodTable;
    Vector3Dd normalVector;
    double Distance;
    double VPNormDotOrigin;
    unsigned int VPCached : 1;
    unsigned int Dominant_Axis : 2;
    unsigned int Inverted : 1;
    unsigned int vAxis : 2;
    Vector3Dd P1;
    Vector3Dd P2;
    Vector3Dd P3;
    short int degenerateFlag;

    static int computeTriangle(Triangle *triangle);
    static int allTriangleIntersections(SimpleBody *object,
        RayWithSegments *ray, PriorityQueueNode *depthQueue);
    static Intersection *objectIntersect(SimpleBody *object, RayWithSegments *ray);
    static int intersectTriangle(
        RayWithSegments *ray, Triangle *triangle, double *depth);
    static int insideTriangle(Vector3Dd *point, SimpleBody *object);
    static void triangleNormal(
        Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint);
    static void *copyTriangle(SimpleBody *object);
    static void translateTriangle(SimpleBody *object, Vector3Dd *vector);
    static void rotateTriangle(SimpleBody *object, Vector3Dd *vector);
    static void scaleTriangle(SimpleBody *object, Vector3Dd *vector);
    static void invertTriangle(SimpleBody *object);

  private:
    static int max3Axis(double x, double y, double z);
    static void findTriangleDominantAxis(Triangle *triangle);
    static void computeSmoothTriangle(SmoothTriangle *triangle);
};

#include "environment/geometry/elements/SmoothTriangle.h"

#endif

#ifndef __TRIANGLE_H__
#define __TRIANGLE_H__

#include "common/Frame.h"
#include "common/Vector.h"
#include "geom/Geometry.h"

class SmoothTriangle;

class Triangle : public Geometry {
  public:
    Vector3D Normal_Vector;
    double Distance;
    double VPNormDotOrigin;
    unsigned int VPCached : 1;
    unsigned int Dominant_Axis : 2;
    unsigned int Inverted : 1;
    unsigned int vAxis : 2;
    Vector3D P1, P2, P3;
    short int Degenerate_Flag;

    static int computeTriangle(Triangle *triangle);
    static int allTriangleIntersections(
        SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue);
    static int intersectTriangle(Ray *ray, Triangle *triangle, double *depth);
    static int insideTriangle(Vector3D *point, SimpleBody *object);
    static void triangleNormal(
        Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint);
    static void *copyTriangle(SimpleBody *object);
    static void translateTriangle(SimpleBody *object, Vector3D *vector);
    static void rotateTriangle(SimpleBody *object, Vector3D *vector);
    static void scaleTriangle(SimpleBody *object, Vector3D *vector);
    static void invertTriangle(SimpleBody *object);

  private:
    static int max3Axis(double x, double y, double z);
    static void findTriangleDominantAxis(Triangle *triangle);
    static void computeSmoothTriangle(SmoothTriangle *triangle);
};

#include "geom/SmoothTriangle.h"

extern Methods Triangle_Methods;
extern Triangle *getTriangleShape(void);
extern SmoothTriangle *getSmoothTriangleShape(void);
extern Methods Smooth_Triangle_Methods;

#endif

#ifndef __SMOOTH_TRIANGLE_H__
#define __SMOOTH_TRIANGLE_H__

#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

class SmoothTriangle : public Geometry {
  public:
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
    bool degenerateFlag;
    Vector3Dd N1;
    Vector3Dd N2;
    Vector3Dd N3;
    Vector3Dd Perp;
    double BaseDelta;

    static void smoothTriangleNormal(
        Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint);
    static void *copySmoothTriangle(SimpleBody *object);
    static void translateSmoothTriangle(SimpleBody *object, Vector3Dd *vector);
    static void rotateSmoothTriangle(SimpleBody *object, Vector3Dd *vector);
    static void scaleSmoothTriangle(SimpleBody *object, Vector3Dd *vector);
    static void invertSmoothTriangle(SimpleBody *object);
};

#endif

#ifndef __SMOOTH_TRIANGLE_H__
#define __SMOOTH_TRIANGLE_H__

#include "common/FrameConfig.h"
#include "common/Vector3Dd.h"
#include "geom/GeometryOperations.h"

class SmoothTriangle : public Geometry {
  public:
    Vector3Dd Normal_Vector;
    double Distance;
    double VPNormDotOrigin;
    unsigned int VPCached : 1;
    unsigned int Dominant_Axis : 2;
    unsigned int Inverted : 1;
    unsigned int vAxis : 2;
    Vector3Dd P1, P2, P3;
    short int Degenerate_Flag;
    Vector3Dd N1, N2, N3, Perp;
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

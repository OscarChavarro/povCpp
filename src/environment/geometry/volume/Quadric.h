#ifndef __QUADRIC_H__
#define __QUADRIC_H__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

class Quadric : public Geometry {
  public:
    static Methods methodTable;
    Vector3Dd object2Terms;
    Vector3Dd objectMixedTerms;
    Vector3Dd objectTerms;
    double objectConstant;
    double objectVpConstant;
    bool constantCached;
    bool nonZeroSquareTerm;

    static int allQuadricIntersections(SimpleBody *object, RayWithSegments *ray,
        PriorityQueueNode *depthQueue);
    static int intersectQuadric(
        RayWithSegments *ray, Quadric *shape, double *depth1, double *depth2);
    static int insideQuadric(Vector3Dd *point, SimpleBody *object);
    static void quadricNormal(
        Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint);
    static void *copyQuadric(SimpleBody *object);
    static void translateQuadric(SimpleBody *object, Vector3Dd *vector);
    static void rotateQuadric(SimpleBody *object, Vector3Dd *vector);
    static void scaleQuadric(SimpleBody *object, Vector3Dd *vector);
    static void invertQuadric(SimpleBody *object);

  private:
    static void quadricToMatrix(const Quadric *quadric, Matrix4x4d *matrix);
    static void matrixToQuadric(const Matrix4x4d *matrix, Quadric *quadric);
    static void transformQuadric(
        Quadric *shape, const Matrix4x4d *transformationInverse);
};
#endif

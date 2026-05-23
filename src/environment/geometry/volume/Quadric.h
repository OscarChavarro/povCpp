#ifndef __QUADRIC_H__
#define __QUADRIC_H__

#include "common/LegacyBoolean.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

class Quadric : public Geometry {
  public:
    Vector3Dd object2Terms;
    Vector3Dd objectMixedTerms;
    Vector3Dd objectTerms;
    double objectConstant;
    double objectVpConstant;
    int constantCached;
    int nonZeroSquareTerm;

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
    static void quadricToMatrix(Quadric *quadric, MATRIX *matrix);
    static void matrixToQuadric(MATRIX *matrix, Quadric *quadric);
    static void transformQuadric(
        Quadric *shape, Transformation *transformation);
};

extern Methods Quadric_Methods;
extern Quadric *getQuadricShape(void);
#endif

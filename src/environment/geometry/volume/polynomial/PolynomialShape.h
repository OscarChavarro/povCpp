#ifndef __POLY_H__
#define __POLY_H__

#include "common/LegacyBoolean.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

class PolynomialShape : public Geometry {
  public:
    Transformation *Transform;
    short Inverted;
    int Order, Sturm_Flag;
    double *Coeffs;

    static int allPolyIntersections(SimpleBody *object, RayWithSegments *ray,
        PriorityQueueNode *depthQueue);
    static int insidePoly(Vector3Dd *point, SimpleBody *object);
    static void polyNormal(
        Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint);
    static void *copyPoly(SimpleBody *object);
    static void translatePoly(SimpleBody *object, Vector3Dd *vector);
    static void rotatePoly(SimpleBody *object, Vector3Dd *vector);
    static void scalePoly(SimpleBody *object, Vector3Dd *vector);
    static void invertPoly(SimpleBody *object);

  private:
    static void transform(int order, double *coeffs, MATRIX *q);
    static int roll(int order, int x, int y, int z);
    static void unroll(int order, int index, int *x, int *y, int *z, int *w);
    static int intersect(
        RayWithSegments *ray, int order, double *coeffs, double *depths);
    static int intersectQuartic(
        RayWithSegments *ray, PolynomialShape *shape, double *depths);
    static void quarticNormal(
        Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint);
    static double inside(Vector3Dd *point, int order, double *coeffs);
    static void normalp(Vector3Dd *result, int order, double *coeffs,
        Vector3Dd *intersectionPoint);
    static double doPartialTerm(
        MATRIX *q, int row, int pwr, int i, int j, int k, int l);
};

extern Methods Poly_Methods;
extern PolynomialShape *getPolyShape(int);

#endif

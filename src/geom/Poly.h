#ifndef __POLY_H__
#define __POLY_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Geometry.h"

class Poly : public Geometry {
  public:
    Transformation *Transform;
    short Inverted;
    int Order, Sturm_Flag;
    DBL *Coeffs;

    static int allPolyIntersections(
        SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue);
    static int insidePoly(Vector3D *point, SimpleBody *object);
    static void polyNormal(
        Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint);
    static void *copyPoly(SimpleBody *object);
    static void translatePoly(SimpleBody *object, Vector3D *vector);
    static void rotatePoly(SimpleBody *object, Vector3D *vector);
    static void scalePoly(SimpleBody *object, Vector3D *vector);
    static void invertPoly(SimpleBody *object);

  private:
    static void transform(int order, DBL *coeffs, MATRIX *q);
    static int roll(int order, int x, int y, int z);
    static void unroll(int order, int index, int *x, int *y, int *z, int *w);
    static int intersect(Ray *ray, int order, DBL *coeffs, DBL *depths);
    static int intersectQuartic(Ray *ray, Poly *shape, DBL *depths);
    static void quarticNormal(
        Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint);
    static DBL inside(Vector3D *point, int order, DBL *coeffs);
    static void normalp(
        Vector3D *result, int order, DBL *coeffs, Vector3D *intersectionPoint);
    static DBL doPartialTerm(
        MATRIX *q, int row, int pwr, int i, int j, int k, int l);
};

extern Methods Poly_Methods;
extern Poly *getPolyShape(int);

#endif

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
};

extern Methods Poly_Methods;
extern Poly *getPolyShape(int);
extern int allPolyIntersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int insidePoly(Vector3D *point, SimpleBody *Object);
extern void polyNormal(
    Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *copyPoly(SimpleBody *Object);
extern void translatePoly(SimpleBody *Object, Vector3D *Vector);
extern void rotatePoly(SimpleBody *Object, Vector3D *Vector);
extern void scalePoly(SimpleBody *Object, Vector3D *Vector);
extern void invertPoly(SimpleBody *Object);

#endif

#ifndef __POLY_H__
#define __POLY_H__

#include "common/frame.h"
#include "common/povproto.h"
#include "common/vector.h"
#include "geom/geometry.h"

class Poly : public Geometry {
  public:
    Transformation *Transform;
    short Inverted;
    int Order, Sturm_Flag;
    DBL *Coeffs;
};

extern Methods Poly_Methods;
extern Poly *Get_Poly_Shape(int);
extern int All_Poly_Intersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int Inside_Poly(Vector3D *point, SimpleBody *Object);
extern void Poly_Normal(
    Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *Copy_Poly(SimpleBody *Object);
extern void Translate_Poly(SimpleBody *Object, Vector3D *Vector);
extern void Rotate_Poly(SimpleBody *Object, Vector3D *Vector);
extern void Scale_Poly(SimpleBody *Object, Vector3D *Vector);
extern void Invert_Poly(SimpleBody *Object);

#endif

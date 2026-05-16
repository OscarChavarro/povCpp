#ifndef __QUADRICS_H__
#define __QUADRICS_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Geometry.h"

class Quadric : public Geometry {
  public:
    Vector3D Object_2_Terms;
    Vector3D Object_Mixed_Terms;
    Vector3D Object_Terms;
    DBL Object_Constant;
    DBL Object_VP_Constant;
    int Constant_Cached;
    int Non_Zero_Square_Term;
};

extern Methods Quadric_Methods;
extern Quadric *Get_Quadric_Shape(void);
extern int All_Quadric_Intersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int Intersect_Quadric(
    Ray *Ray, Quadric *Shape, DBL *Depth1, DBL *Depth2);
extern int Inside_Quadric(Vector3D *point, SimpleBody *Object);
extern void Quadric_Normal(
    Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *Copy_Quadric(SimpleBody *Object);
extern void Translate_Quadric(SimpleBody *Object, Vector3D *Vector);
extern void Rotate_Quadric(SimpleBody *Object, Vector3D *Vector);
extern void Scale_Quadric(SimpleBody *Object, Vector3D *Vector);
extern void Invert_Quadric(SimpleBody *Object);

#endif

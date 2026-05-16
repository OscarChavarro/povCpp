#ifndef __BOX_H__
#define __BOX_H__

#include "common/frame.h"
#include "common/vector.h"
#include "common/povproto.h"
#include "geom/geometry.h"

class Box : public Geometry
{
  public:
    Transformation *Transform;
    Vector3D bounds[2];
    short Inverted;
};

extern Methods Box_Methods;
extern Box *Get_Box_Shape();
extern int All_Box_Intersections(SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int Intersect_Boxx(Ray *Ray, Box *box, DBL *Depth1, DBL *Depth2);
extern int Inside_Box(Vector3D *point, SimpleBody *Object);
extern void Box_Normal(Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *Copy_Box(SimpleBody *Object);
extern void Translate_Box(SimpleBody *Object, Vector3D *Vector);
extern void Rotate_Box(SimpleBody *Object, Vector3D *Vector);
extern void Scale_Box(SimpleBody *Object, Vector3D *Vector);
extern void Invert_Box(SimpleBody *Object);

#endif

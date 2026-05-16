#ifndef __CSG_H__
#define __CSG_H__

#include "common/frame.h"
#include "common/povproto.h"
#include "common/vector.h"
#include "geom/geometry.h"

class CSG : public Geometry {
  public:
    Geometry *Shapes;
};

extern Methods CSG_Union_Methods;
extern Methods CSG_Intersection_Methods;
extern CSG *Get_CSG_Shape(void);
extern CSG *Get_CSG_Union(void);
extern CSG *Get_CSG_Intersection(void);
extern int All_CSG_Union_Intersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int All_CSG_Intersect_Intersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int Inside_CSG_Union(Vector3D *point, SimpleBody *Object);
extern int Inside_CSG_Intersection(Vector3D *point, SimpleBody *Object);
void *Copy_CSG(SimpleBody *Object);
extern void Translate_CSG(SimpleBody *Object, Vector3D *Vector);
extern void Rotate_CSG(SimpleBody *Object, Vector3D *Vector);
extern void Scale_CSG(SimpleBody *Object, Vector3D *Vector);
extern void Invert_CSG(SimpleBody *Object);

#endif

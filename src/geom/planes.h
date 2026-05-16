#ifndef __PLANES_H__
#define __PLANES_H__

#include "common/frame.h"
#include "common/povproto.h"
#include "common/vector.h"
#include "geom/geometry.h"

class InfinitePlane : public Geometry {
  public:
    Vector3D Normal_Vector;
    DBL Distance;
    DBL VPNormDotOrigin;
    int VPCached;
};

extern Methods Plane_Methods;
extern InfinitePlane *Get_Plane_Shape(void);
extern int All_Plane_Intersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int Intersect_Plane(Ray *Ray, InfinitePlane *Plane, DBL *Depth);
extern int Inside_Plane(Vector3D *point, SimpleBody *Object);
extern void Plane_Normal(
    Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *Copy_Plane(SimpleBody *Object);
extern void Translate_Plane(SimpleBody *Object, Vector3D *Vector);
extern void Rotate_Plane(SimpleBody *Object, Vector3D *Vector);
extern void Scale_Plane(SimpleBody *Object, Vector3D *Vector);
extern void Invert_Plane(SimpleBody *Object);

#endif

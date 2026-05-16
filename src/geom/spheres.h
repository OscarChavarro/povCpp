#ifndef __SPHERES_H__
#define __SPHERES_H__

#include "common/frame.h"
#include "common/vector.h"
#include "common/povproto.h"
#include "geom/geometry.h"

class Sphere : public Geometry
{
  public:
    Vector3D  Center;
    DBL      Radius;
    DBL      Radius_Squared;
    DBL      Inverse_Radius;
    Vector3D  VPOtoC;
    DBL      VPOCSquared;
    short    VPinside, VPCached, Inverted;
};

extern Methods Sphere_Methods;
extern Sphere *Get_Sphere_Shape(void);
extern int All_Sphere_Intersections(SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int Intersect_Sphere(Ray *Ray, Sphere *Sphere, DBL *Depth1, DBL *Depth2);
extern int Inside_Sphere(Vector3D *point, SimpleBody *Object);
extern void Sphere_Normal(Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *Copy_Sphere(SimpleBody *Object);
extern void Translate_Sphere(SimpleBody *Object, Vector3D *Vector);
extern void Rotate_Sphere(SimpleBody *Object, Vector3D *Vector);
extern void Scale_Sphere(SimpleBody *Object, Vector3D *Vector);
extern void Invert_Sphere(SimpleBody *Object);

#endif

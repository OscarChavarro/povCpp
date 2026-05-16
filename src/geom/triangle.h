#ifndef __TRIANGLE_H__
#define __TRIANGLES_H__

#include "common/frame.h"
#include "common/povproto.h"
#include "common/vector.h"
#include "geom/geometry.h"

#include "geom/TriangleClass.h"
#include "geom/SmoothTriangle.h"

extern Methods Triangle_Methods;
extern Triangle *Get_Triangle_Shape(void);
extern SmoothTriangle *Get_Smooth_Triangle_Shape(void);
extern Methods Smooth_Triangle_Methods;
extern int Compute_Triangle(Triangle *Triangle);
extern int All_Triangle_Intersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int Intersect_Triangle(Ray *Ray, Triangle *Triangle, DBL *Depth);
extern int Inside_Triangle(Vector3D *point, SimpleBody *Object);
extern void Triangle_Normal(
    Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *Copy_Triangle(SimpleBody *Object);
extern void Translate_Triangle(SimpleBody *Object, Vector3D *Vector);
extern void Rotate_Triangle(SimpleBody *Object, Vector3D *Vector);
extern void Scale_Triangle(SimpleBody *Object, Vector3D *Vector);
extern void Invert_Triangle(SimpleBody *Object);
extern void Smooth_Triangle_Normal(
    Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *Copy_Smooth_Triangle(SimpleBody *Object);
extern void Translate_Smooth_Triangle(SimpleBody *Object, Vector3D *Vector);
extern void Rotate_Smooth_Triangle(SimpleBody *Object, Vector3D *Vector);
extern void Scale_Smooth_Triangle(SimpleBody *Object, Vector3D *Vector);
extern void Invert_Smooth_Triangle(SimpleBody *Object);

#endif

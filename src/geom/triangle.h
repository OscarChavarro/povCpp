#ifndef __TRIANGLE_H__
#define __TRIANGLES_H__

#include "common/frame.h"
#include "common/vector.h"
#include "common/povproto.h"
#include "geom/geometry.h"

class Triangle : public Geometry
{
  public:
    Vector3D  Normal_Vector;
    DBL      Distance;
    DBL      VPNormDotOrigin;
    unsigned int VPCached:1;
    unsigned int Dominant_Axis:2;
    unsigned int Inverted:1;
    unsigned int vAxis:2;  /* used only for smooth triangles */
    Vector3D  P1, P2, P3;
    short int Degenerate_Flag;
};

class SmoothTriangle : public Geometry
{
  public:
    Vector3D  Normal_Vector;
    DBL      Distance;
    DBL      VPNormDotOrigin;
    unsigned int VPCached:1;
    unsigned int Dominant_Axis:2;
    unsigned int Inverted:1;
    unsigned int vAxis:2;            /* used only for smooth triangles */
    Vector3D  P1, P2, P3;
    short int Degenerate_Flag;  /* do not move this field */
    Vector3D  N1, N2, N3, Perp;
    DBL  BaseDelta;
};

extern Methods Triangle_Methods;
extern Triangle *Get_Triangle_Shape(void);
extern SmoothTriangle *Get_Smooth_Triangle_Shape(void);
extern Methods Smooth_Triangle_Methods;
extern int Compute_Triangle (Triangle *Triangle);
extern int All_Triangle_Intersections (SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int Intersect_Triangle (Ray *Ray, Triangle *Triangle, DBL *Depth);
extern int Inside_Triangle (Vector3D *point, SimpleBody *Object);
extern void Triangle_Normal (Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *Copy_Triangle (SimpleBody *Object);
extern void Translate_Triangle (SimpleBody *Object, Vector3D *Vector);
extern void Rotate_Triangle (SimpleBody *Object, Vector3D *Vector);
extern void Scale_Triangle (SimpleBody *Object, Vector3D *Vector);
extern void Invert_Triangle (SimpleBody *Object);
extern void Smooth_Triangle_Normal (Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *Copy_Smooth_Triangle(SimpleBody *Object);
extern void Translate_Smooth_Triangle (SimpleBody *Object, Vector3D *Vector);
extern void Rotate_Smooth_Triangle (SimpleBody *Object, Vector3D *Vector);
extern void Scale_Smooth_Triangle (SimpleBody *Object, Vector3D *Vector);
extern void Invert_Smooth_Triangle (SimpleBody *Object);

#endif

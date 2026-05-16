#ifndef __POINT_H__
#define __POINT_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Geometry.h"

class Light : public Geometry {
  public:
    Vector3D Center, Points_At;
    Light *Next_Light_Source;
    short Inverted;
    DBL Coeff, Radius, Falloff;
};

extern Methods Point_Methods;
extern Light *Get_Light_Source_Shape(void);
extern int All_Point_Intersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int Inside_Point(Vector3D *Test_Point, SimpleBody *Object);
extern void *Copy_Point(SimpleBody *Object);
extern void Translate_Point(SimpleBody *Object, Vector3D *Vector);
extern void Rotate_Point(SimpleBody *Object, Vector3D *Vector);
extern void Scale_Point(SimpleBody *Object, Vector3D *Vector);
extern void Invert_Point(SimpleBody *Object);
extern DBL Attenuate_Light(Light *Light_Source, Ray *Light_Source_Ray);

#endif

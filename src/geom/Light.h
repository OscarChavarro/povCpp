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
extern Light *getLightSourceShape(void);
extern int allPointIntersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int insidePoint(Vector3D *Test_Point, SimpleBody *Object);
extern void *copyPoint(SimpleBody *Object);
extern void translatePoint(SimpleBody *Object, Vector3D *Vector);
extern void rotatePoint(SimpleBody *Object, Vector3D *Vector);
extern void scalePoint(SimpleBody *Object, Vector3D *Vector);
extern void invertPoint(SimpleBody *Object);
extern DBL attenuateLight(Light *Light_Source, Ray *Light_Source_Ray);

#endif

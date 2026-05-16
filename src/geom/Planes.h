#ifndef __PLANES_H__
#define __PLANES_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Geometry.h"

class InfinitePlane : public Geometry {
  public:
    Vector3D Normal_Vector;
    DBL Distance;
    DBL VPNormDotOrigin;
    int VPCached;
};

extern Methods Plane_Methods;
extern InfinitePlane *getPlaneShape(void);
extern int allPlaneIntersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int intersectPlane(Ray *Ray, InfinitePlane *Plane, DBL *Depth);
extern int insidePlane(Vector3D *point, SimpleBody *Object);
extern void planeNormal(
    Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *copyPlane(SimpleBody *Object);
extern void translatePlane(SimpleBody *Object, Vector3D *Vector);
extern void rotatePlane(SimpleBody *Object, Vector3D *Vector);
extern void scalePlane(SimpleBody *Object, Vector3D *Vector);
extern void invertPlane(SimpleBody *Object);

#endif

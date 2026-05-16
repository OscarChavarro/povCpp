#ifndef __SPHERES_H__
#define __SPHERES_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Geometry.h"

class Sphere : public Geometry {
  public:
    Vector3D Center;
    DBL Radius;
    DBL Radius_Squared;
    DBL Inverse_Radius;
    Vector3D VPOtoC;
    DBL VPOCSquared;
    short VPinside, VPCached, Inverted;
};

extern Methods Sphere_Methods;
extern Sphere *getSphereShape(void);
extern int allSphereIntersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int intersectSphere(Ray *Ray, Sphere *Sphere, DBL *Depth1, DBL *Depth2);
extern int insideSphere(Vector3D *point, SimpleBody *Object);
extern void sphereNormal(
    Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *copySphere(SimpleBody *Object);
extern void translateSphere(SimpleBody *Object, Vector3D *Vector);
extern void rotateSphere(SimpleBody *Object, Vector3D *Vector);
extern void scaleSphere(SimpleBody *Object, Vector3D *Vector);
extern void invertSphere(SimpleBody *Object);

#endif

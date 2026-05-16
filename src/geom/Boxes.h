#ifndef __BOX_H__
#define __BOX_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Geometry.h"

class Box : public Geometry {
  public:
    Transformation *Transform;
    Vector3D bounds[2];
    short Inverted;
};

extern Methods Box_Methods;
extern Box *getBoxShape();
extern int allBoxIntersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int intersectBoxx(Ray *Ray, Box *box, DBL *Depth1, DBL *Depth2);
extern int insideBox(Vector3D *point, SimpleBody *Object);
extern void boxNormal(
    Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *copyBox(SimpleBody *Object);
extern void translateBox(SimpleBody *Object, Vector3D *Vector);
extern void rotateBox(SimpleBody *Object, Vector3D *Vector);
extern void scaleBox(SimpleBody *Object, Vector3D *Vector);
extern void invertBox(SimpleBody *Object);

#endif

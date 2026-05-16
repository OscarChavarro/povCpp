#ifndef __CSG_H__
#define __CSG_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Geometry.h"

class CSG : public Geometry {
  public:
    Geometry *Shapes;
};

extern Methods CSG_Union_Methods;
extern Methods CSG_Intersection_Methods;
extern CSG *getCsgShape(void);
extern CSG *getCsgUnion(void);
extern CSG *getCsgIntersection(void);
extern int allCsgUnionIntersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int allCsgIntersectIntersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int insideCsgUnion(Vector3D *point, SimpleBody *Object);
extern int insideCsgIntersection(Vector3D *point, SimpleBody *Object);
void *copyCsg(SimpleBody *Object);
extern void translateCsg(SimpleBody *Object, Vector3D *Vector);
extern void rotateCsg(SimpleBody *Object, Vector3D *Vector);
extern void scaleCsg(SimpleBody *Object, Vector3D *Vector);
extern void invertCsg(SimpleBody *Object);

#endif

#ifndef __QUADRICS_H__
#define __QUADRICS_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Geometry.h"

class Quadric : public Geometry {
  public:
    Vector3D Object_2_Terms;
    Vector3D Object_Mixed_Terms;
    Vector3D Object_Terms;
    DBL Object_Constant;
    DBL Object_VP_Constant;
    int Constant_Cached;
    int Non_Zero_Square_Term;
};

extern Methods Quadric_Methods;
extern Quadric *getQuadricShape(void);
extern int allQuadricIntersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int intersectQuadric(
    Ray *Ray, Quadric *Shape, DBL *Depth1, DBL *Depth2);
extern int insideQuadric(Vector3D *point, SimpleBody *Object);
extern void quadricNormal(
    Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *copyQuadric(SimpleBody *Object);
extern void translateQuadric(SimpleBody *Object, Vector3D *Vector);
extern void rotateQuadric(SimpleBody *Object, Vector3D *Vector);
extern void scaleQuadric(SimpleBody *Object, Vector3D *Vector);
extern void invertQuadric(SimpleBody *Object);

#endif

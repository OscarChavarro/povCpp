#ifndef __TRIANGLE_H__
#define __TRIANGLES_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Geometry.h"

#include "geom/SmoothTriangle.h"
#include "geom/TriangleClass.h"

extern Methods Triangle_Methods;
extern Triangle *getTriangleShape(void);
extern SmoothTriangle *getSmoothTriangleShape(void);
extern Methods Smooth_Triangle_Methods;
extern int computeTriangle(Triangle *Triangle);
extern int allTriangleIntersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int intersectTriangle(Ray *Ray, Triangle *Triangle, DBL *Depth);
extern int insideTriangle(Vector3D *point, SimpleBody *Object);
extern void triangleNormal(
    Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *copyTriangle(SimpleBody *Object);
extern void translateTriangle(SimpleBody *Object, Vector3D *Vector);
extern void rotateTriangle(SimpleBody *Object, Vector3D *Vector);
extern void scaleTriangle(SimpleBody *Object, Vector3D *Vector);
extern void invertTriangle(SimpleBody *Object);
extern void smoothTriangleNormal(
    Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *copySmoothTriangle(SimpleBody *Object);
extern void translateSmoothTriangle(SimpleBody *Object, Vector3D *Vector);
extern void rotateSmoothTriangle(SimpleBody *Object, Vector3D *Vector);
extern void scaleSmoothTriangle(SimpleBody *Object, Vector3D *Vector);
extern void invertSmoothTriangle(SimpleBody *Object);

#endif

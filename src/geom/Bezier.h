#ifndef __BEZIER_H__
#define __BEZIER_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Geometry.h"

#define BEZIER_INTERIOR_NODE 0
#define BEZIER_LEAF_NODE 1

class BezierVertices;
class BezierNode;
#include "geom/BezierChild.h"
#include "geom/BezierNode.h"
#include "geom/BezierVertices.h"
#include "geom/BicubicPatch.h"

extern Methods Bicubic_Patch_Methods;
extern BicubicPatch *getBicubicPatchShape();
extern void precomputePatchValues(BicubicPatch *Shape);
extern int allBicubicPatchIntersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int insideBicubicPatch(Vector3D *point, SimpleBody *Object);
extern void bicubicPatchNormal(
    Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *copyBicubicPatch(SimpleBody *Object);
extern void translateBicubicPatch(SimpleBody *Object, Vector3D *Vector);
extern void rotateBicubicPatch(SimpleBody *Object, Vector3D *Vector);
extern void scaleBicubicPatch(SimpleBody *Object, Vector3D *Vector);
extern void invertBicubicPatch(SimpleBody *Object);

#endif

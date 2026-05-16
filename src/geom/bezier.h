#ifndef __BEZIER_H__
#define __BEZIER_H__

#include "common/frame.h"
#include "common/povproto.h"
#include "common/vector.h"
#include "geom/geometry.h"

#define BEZIER_INTERIOR_NODE 0
#define BEZIER_LEAF_NODE 1

class BezierVertices;
class BezierNode;
#include "geom/BezierChild.h"
#include "geom/BezierVertices.h"
#include "geom/BezierNode.h"
#include "geom/BicubicPatch.h"

extern Methods Bicubic_Patch_Methods;
extern BicubicPatch *Get_Bicubic_Patch_Shape();
extern void Precompute_Patch_Values(BicubicPatch *Shape);
extern int All_Bicubic_Patch_Intersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int Inside_Bicubic_Patch(Vector3D *point, SimpleBody *Object);
extern void Bicubic_Patch_Normal(
    Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *Copy_Bicubic_Patch(SimpleBody *Object);
extern void Translate_Bicubic_Patch(SimpleBody *Object, Vector3D *Vector);
extern void Rotate_Bicubic_Patch(SimpleBody *Object, Vector3D *Vector);
extern void Scale_Bicubic_Patch(SimpleBody *Object, Vector3D *Vector);
extern void Invert_Bicubic_Patch(SimpleBody *Object);

#endif

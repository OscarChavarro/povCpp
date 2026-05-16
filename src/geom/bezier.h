#ifndef __BEZIER_H__
#define __BEZIER_H__

#include "common/frame.h"
#include "common/vector.h"
#include "common/povproto.h"
#include "geom/geometry.h"

#define BEZIER_INTERIOR_NODE 0
#define BEZIER_LEAF_NODE 1
#define MAX_BICUBIC_INTERSECTIONS 32

class BezierVertices;
class BezierNode;

class BezierChild
{
  public:
    BezierNode *Children[4];
};

class BezierVertices
{
  public:
    Vector3D Vertices[4];
};

class BezierNode
{
  public:
    int Node_Type;        /* Is this an interior node, or a leaf */
    Vector3D Center;        /* Center of sphere bounding the (sub)patch */
    DBL Radius_Squared; /* Radius of bounding sphere (squared) */
    int Count;             /* # of subpatches associated with this node */
    void *Data_Ptr;      /* Either pointer to vertices or pointer to children */
};

class BicubicPatch : public Geometry
{
  public:
    int Patch_Type, U_Steps, V_Steps;
    Vector3D Control_Points[4][4];
    Vector3D Bounding_Sphere_Center;
    DBL Bounding_Sphere_Radius;
    DBL Flatness_Value;
    int Intersection_Count;
    Vector3D Normal_Vector[MAX_BICUBIC_INTERSECTIONS];
    Vector3D Intersection_Point[MAX_BICUBIC_INTERSECTIONS];
    Vector3D **Interpolated_Grid, **Interpolated_Normals, **Smooth_Normals;
    DBL **Interpolated_D;
    BezierNode *Node_Tree;
};

extern Methods Bicubic_Patch_Methods;
extern BicubicPatch *Get_Bicubic_Patch_Shape();
extern void Precompute_Patch_Values(BicubicPatch *Shape);
extern int All_Bicubic_Patch_Intersections(SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue);
extern int Inside_Bicubic_Patch(Vector3D *point, SimpleBody *Object);
extern void Bicubic_Patch_Normal(Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point);
extern void *Copy_Bicubic_Patch(SimpleBody *Object);
extern void Translate_Bicubic_Patch(SimpleBody *Object, Vector3D *Vector);
extern void Rotate_Bicubic_Patch(SimpleBody *Object, Vector3D *Vector);
extern void Scale_Bicubic_Patch(SimpleBody *Object, Vector3D *Vector);
extern void Invert_Bicubic_Patch(SimpleBody *Object);

#endif

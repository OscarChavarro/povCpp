#ifndef __BICUBIC_PATCH_H__
#define __BICUBIC_PATCH_H__

#include "common/Frame.h"
#include "common/Vector.h"
#include "geom/BezierNode.h"
#include "geom/Geometry.h"

#define MAX_BICUBIC_INTERSECTIONS 32

class BicubicPatch : public Geometry {
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

#endif

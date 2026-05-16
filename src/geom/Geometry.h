#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include "common/Matrices.h"
#include "common/Ray.h"

/*===========================================================================*/

/* Object/shape types */
#define SPHERE_TYPE 0
#define TRIANGLE_TYPE 1
#define SMOOTH_TRIANGLE_TYPE 2
#define PLANE_TYPE 3
#define QUADRIC_TYPE 4
#define POLY_TYPE 5
#define BICUBIC_PATCH_TYPE 6
#define COMPOSITE_TYPE 7
#define OBJECT_TYPE 8
#define CSG_UNION_TYPE 9
#define CSG_INTERSECTION_TYPE 10
#define CSG_DIFFERENCE_TYPE 11
#define VIEWPOINT_TYPE 12
#define HEIGHT_FIELD_TYPE 13
#define POINT_LIGHT_TYPE 14
#define SPOT_LIGHT_TYPE 15
#define BOX_TYPE 16
#define BLOB_TYPE 17

class PriorityQueueNode;
class Intersection;
class Methods;
class Geometry;
class SimpleBody;

#define All_Intersections(x, y, z)                                             \
    ((*((x)->methods->All_Intersections_Method))(x, y, z))
#define Intersection(x, y)                                                     \
    ((Intersection *)((*((x)->methods->Intersection_Method))(x, y)))
#define Inside(x, y) ((*((y)->methods->Inside_Method))(x, y))
#define Normal(x, y, z) ((*((y)->methods->Normal_Method))(x, y, z))
#define Copy(x) ((*((x)->methods->Copy_Method))(x))
#define Translate(x, y) ((*((x)->methods->Translate_Method))(x, y))
#define Scale(x, y) ((*((x)->methods->Scale_Method))(x, y))
#define Rotate(x, y) ((*((x)->methods->Rotate_Method))(x, y))
#define Invert(x) ((*((x)->methods->Invert_Method))(x))

#include "geom/GeometryClass.h"
#include "geom/MethodsClass.h"
#include "geom/SimpleBody.h"

#include "geom/PrioQ.h"
#include "media/Texture.h"

#endif

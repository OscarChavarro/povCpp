#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include "common/Matrices.h"
#include "common/Ray.h"

/*===========================================================================*/

/* Object/shape types */
static constexpr int SPHERE_TYPE = 0;
static constexpr int TRIANGLE_TYPE = 1;
static constexpr int SMOOTH_TRIANGLE_TYPE = 2;
static constexpr int PLANE_TYPE = 3;
static constexpr int QUADRIC_TYPE = 4;
static constexpr int POLY_TYPE = 5;
static constexpr int BICUBIC_PATCH_TYPE = 6;
static constexpr int COMPOSITE_TYPE = 7;
static constexpr int OBJECT_TYPE = 8;
static constexpr int CSG_UNION_TYPE = 9;
static constexpr int CSG_INTERSECTION_TYPE = 10;
static constexpr int CSG_DIFFERENCE_TYPE = 11;
static constexpr int VIEWPOINT_TYPE = 12;
static constexpr int HEIGHT_FIELD_TYPE = 13;
static constexpr int POINT_LIGHT_TYPE = 14;
static constexpr int SPOT_LIGHT_TYPE = 15;
static constexpr int BOX_TYPE = 16;
static constexpr int BLOB_TYPE = 17;

class PriorityQueueNode;
class Intersection;
class Methods;
class Geometry;
class SimpleBody;

#define allIntersections(x, y, z)                                             \
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

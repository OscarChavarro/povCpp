#ifndef __GEOMETRY_OPERATIONS_H__
#define __GEOMETRY_OPERATIONS_H__

#include "common/Transformation.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/elements/RayWithSegments.h"

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

class Intersection;
class Methods;
class Geometry;
class SimpleBody;

#include "environment/geometry/Intersection.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/Methods.h"
#include "environment/geometry/SimpleBody.h"

#include "common/dataStructures/PriorityQueue.h"
#include "media/Texture.h"

class GeometryOperations {
  public:
    static inline int
    allIntersections(SimpleBody *x, RayWithSegments *y, PriorityQueueNode *z)
    {
        return ((*((x)->methods->All_Intersections_Method))(x, y, z));
    }

    static inline Intersection *
    intersect(SimpleBody *x, RayWithSegments *y)
    {
        return (Intersection *)((*((x)->methods->Intersection_Method))(x, y));
    }

    static inline int
    inside(Vector3Dd *x, SimpleBody *y)
    {
        return ((*((y)->methods->Inside_Method))(x, y));
    }

    static inline void
    normal(Vector3Dd *x, SimpleBody *y, Vector3Dd *z)
    {
        ((*((y)->methods->Normal_Method))(x, y, z));
    }

    static inline void *
    copy(SimpleBody *x)
    {
        return ((*((x)->methods->Copy_Method))(x));
    }

    static inline void
    translate(SimpleBody *x, Vector3Dd *y)
    {
        ((*((x)->methods->Translate_Method))(x, y));
    }

    static inline void
    scale(SimpleBody *x, Vector3Dd *y)
    {
        ((*((x)->methods->Scale_Method))(x, y));
    }

    static inline void
    rotate(SimpleBody *x, Vector3Dd *y)
    {
        ((*((x)->methods->Rotate_Method))(x, y));
    }

    static inline void
    invert(SimpleBody *x)
    {
        ((*((x)->methods->Invert_Method))(x));
    }
};

#endif

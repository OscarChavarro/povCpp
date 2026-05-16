#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include "common/matrices.h"
#include "common/ray.h"

/*===========================================================================*/

/* Object/shape types */
#define SPHERE_TYPE                0
#define TRIANGLE_TYPE             1
#define SMOOTH_TRIANGLE_TYPE    2
#define PLANE_TYPE                 3
#define QUADRIC_TYPE              4
#define POLY_TYPE                  5
#define BICUBIC_PATCH_TYPE      6
#define COMPOSITE_TYPE            7
#define OBJECT_TYPE                8
#define CSG_UNION_TYPE            9
#define CSG_INTERSECTION_TYPE 10
#define CSG_DIFFERENCE_TYPE    11
#define VIEWPOINT_TYPE          12
#define HEIGHT_FIELD_TYPE      13
#define POINT_LIGHT_TYPE        14
#define SPOT_LIGHT_TYPE         15
#define BOX_TYPE                  16
#define BLOB_TYPE                 17

class PriorityQueueNode;
class Intersection;
class Methods;
class Geometry;

class SimpleBody
{
  public:
    Methods *methods;
    int Type;
    SimpleBody *Next_Object;
    /*SimpleBody *Next_Light_Source; */
    Geometry *Bounding_Shapes;
    Geometry *Clipping_Shapes;
    Geometry *Shape;
    char No_Shadow_Flag;
    RGBAColor *Object_Colour;
    Texture *Object_Texture;
};

//===========================================================================
// Note how this code implements object oriented inheritance model for C
// All of these should be methods of a "Geometry" superclass

typedef Intersection * (*INTERSECTION_METHOD)(SimpleBody *, Ray *);
typedef int                (*ALL_INTERSECTIONS_METHOD)(SimpleBody *, Ray *, PriorityQueueNode *);
typedef int                (*INSIDE_METHOD)(Vector3D *, SimpleBody *);
typedef void              (*NORMAL_METHOD)(Vector3D *, SimpleBody *, Vector3D *);
typedef void *            (*COPY_METHOD)(SimpleBody *);
typedef void              (*TRANSLATE_METHOD)(SimpleBody *, Vector3D *);
typedef void              (*ROTATE_METHOD)(SimpleBody *, Vector3D *);
typedef void              (*SCALE_METHOD)(SimpleBody *, Vector3D *);
typedef void              (*INVERT_METHOD)(SimpleBody *);

class Methods
{
  public:
    INTERSECTION_METHOD Intersection_Method;
    ALL_INTERSECTIONS_METHOD All_Intersections_Method;
    INSIDE_METHOD Inside_Method;
    NORMAL_METHOD Normal_Method;
    COPY_METHOD Copy_Method;
    TRANSLATE_METHOD Translate_Method;
    ROTATE_METHOD Rotate_Method;
    SCALE_METHOD Scale_Method;
    INVERT_METHOD Invert_Method;
};

#define All_Intersections(x,y,z) ((*((x)->methods->All_Intersections_Method)) (x,y,z))
#define Intersection(x,y) ((Intersection *) ((*((x)->methods->Intersection_Method)) (x,y)))
#define Inside(x,y) ((*((y)->methods->Inside_Method)) (x,y))
#define Normal(x,y,z) ((*((y)->methods->Normal_Method)) (x,y,z))
#define Copy(x) ((*((x)->methods->Copy_Method)) (x))
#define Translate(x,y) ((*((x)->methods->Translate_Method)) (x,y))
#define Scale(x,y) ((*((x)->methods->Scale_Method)) (x,y))
#define Rotate(x,y) ((*((x)->methods->Rotate_Method)) (x,y))
#define Invert(x) ((*((x)->methods->Invert_Method)) (x))

//===========================================================================

class Geometry
{
  public:
    Methods *methods;
    int Type;
    Geometry *Next_Object;
    SimpleBody *Parent_Object;
    Texture *Shape_Texture;
    RGBAColor *Shape_Colour;
};

#include "geom/prioq.h"
#include "media/texture.h"

#endif

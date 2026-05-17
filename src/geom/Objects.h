#ifndef __OBJECTS_H__
#define __OBJECTS_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Csg.h"
#include "geom/Geometry.h"

class Composite {
  public:
    Methods *methods;
    int Type;
    SimpleBody *Next_Object;
    /*    SimpleBody *Next_Light_Source;*/
    Geometry *Bounding_Shapes;
    Geometry *Clipping_Shapes;
    SimpleBody *Objects;

    static Intersection *objectIntersect(SimpleBody *object, Ray *ray);
    static int allCompositeIntersections(
        SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue);
    static int allObjectIntersections(
        SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue);
    static int insideBasicObject(Vector3D *point, SimpleBody *object);
    static int insideCompositeObject(Vector3D *point, SimpleBody *object);
    static void *copyBasicObject(SimpleBody *object);
    static void *copyCompositeObject(SimpleBody *object);
    static void translateBasicObject(SimpleBody *object, Vector3D *vector);
    static void rotateBasicObject(SimpleBody *object, Vector3D *vector);
    static void scaleBasicObject(SimpleBody *object, Vector3D *vector);
    static void translateCompositeObject(SimpleBody *object, Vector3D *vector);
    static void rotateCompositeObject(SimpleBody *object, Vector3D *vector);
    static void scaleCompositeObject(SimpleBody *object, Vector3D *vector);
    static void invertBasicObject(SimpleBody *object);
    static void invertCompositeObject(SimpleBody *object);
};

extern Methods Composite_Methods;
extern Methods Basic_Object_Methods;
extern Intersection *objectIntersect(SimpleBody *Object, Ray *Ray);
extern void setCsgParents(CSG *Shape, SimpleBody *Object);

extern void Link(
    SimpleBody *New_Object, SimpleBody **Field, SimpleBody **Old_Object_List);

extern SimpleBody *getObject(void);
extern Composite *getCompositeObject(void);

#endif

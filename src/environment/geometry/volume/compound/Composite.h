#ifndef __COMPOSITE_H__
#define __COMPOSITE_H__

#include "common/FrameConfig.h"
#include "app/PovApp.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/scene/ObjectUtils.h"

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
    static int insideBasicObject(Vector3Dd *point, SimpleBody *object);
    static int insideCompositeObject(Vector3Dd *point, SimpleBody *object);
    static void *copyBasicObject(SimpleBody *object);
    static void *copyCompositeObject(SimpleBody *object);
    static void translateBasicObject(SimpleBody *object, Vector3Dd *vector);
    static void rotateBasicObject(SimpleBody *object, Vector3Dd *vector);
    static void scaleBasicObject(SimpleBody *object, Vector3Dd *vector);
    static void translateCompositeObject(SimpleBody *object, Vector3Dd *vector);
    static void rotateCompositeObject(SimpleBody *object, Vector3Dd *vector);
    static void scaleCompositeObject(SimpleBody *object, Vector3Dd *vector);
    static void invertBasicObject(SimpleBody *object);
    static void invertCompositeObject(SimpleBody *object);
};

extern Methods Composite_Methods;
extern Methods Basic_Object_Methods;
extern void setCsgParents(CSG *Shape, SimpleBody *Object);

#endif

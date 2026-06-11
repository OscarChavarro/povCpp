#ifndef __COMPOSITE_H__
#define __COMPOSITE_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/GeometryTypes.h"
#include "environment/geometry/volume/compound/CSG.h"

class Composite {
  public:
    static Methods compositeMethodTable;
    static Methods basicObjectMethodTable;
    Methods *methods;
    SimpleBody *nextObject;
    Geometry *boundingShapes;
    Geometry *clippingShapes;
    SimpleBody *simpleBodies;

    static int allCompositeIntersections(SimpleBody *object,
        RayWithSegments *ray, PriorityQueueNode *depthQueue);
    static int allObjectIntersections(SimpleBody *object, RayWithSegments *ray,
        PriorityQueueNode *depthQueue);
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

  private:
    static inline void linkSimpleBody(
        SimpleBody *newObject, SimpleBody **field, SimpleBody **oldObjectList);
    static SimpleBody *createBasicObject();
};

#endif

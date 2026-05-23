#ifndef __METHODS_H__
#define __METHODS_H__

#include "common/dataStructures/PriorityQueueNode.h"
#include "common/linealAlgebra/Transformation.h"
#include "environment/geometry/elements/RayWithSegments.h"

class Intersection;
class SimpleBody;

typedef Intersection *(*INTERSECTION_METHOD)(SimpleBody *, RayWithSegments *);
typedef int (*ALL_INTERSECTIONS_METHOD)(
    SimpleBody *, RayWithSegments *, PriorityQueueNode *);
typedef int (*INSIDE_METHOD)(Vector3Dd *, SimpleBody *);
typedef void (*NORMAL_METHOD)(Vector3Dd *, SimpleBody *, Vector3Dd *);
typedef void *(*COPY_METHOD)(SimpleBody *);
typedef void (*TRANSLATE_METHOD)(SimpleBody *, Vector3Dd *);
typedef void (*ROTATE_METHOD)(SimpleBody *, Vector3Dd *);
typedef void (*SCALE_METHOD)(SimpleBody *, Vector3Dd *);
typedef void (*INVERT_METHOD)(SimpleBody *);

class Methods {
  public:
    INTERSECTION_METHOD intersectionMethod;
    ALL_INTERSECTIONS_METHOD allIntersectionsMethod;
    INSIDE_METHOD insideMethod;
    NORMAL_METHOD normalMethod;
    COPY_METHOD copyMethod;
    TRANSLATE_METHOD translateMethod;
    ROTATE_METHOD rotateMethod;
    SCALE_METHOD scaleMethod;
    INVERT_METHOD invertMethod;
};

#endif

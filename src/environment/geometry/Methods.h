#ifndef __METHODS_H__
#define __METHODS_H__

#include "environment/geometry/Intersection.h"
#include "environment/geometry/SimpleBody.h"
#include "common/dataStructures/PriorityQueueNode.h"
#include "environment/geometry/elements/RayWithSegments.h"

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

#ifndef __METHODS_H__
#define __METHODS_H__

#include "common/Transformation.h"
#include "common/Ray.h"

class PriorityQueueNode;
class Intersection;
class SimpleBody;

typedef Intersection *(*INTERSECTION_METHOD)(SimpleBody *, Ray *);
typedef int (*ALL_INTERSECTIONS_METHOD)(
    SimpleBody *, Ray *, PriorityQueueNode *);
typedef int (*INSIDE_METHOD)(Vector3Dd *, SimpleBody *);
typedef void (*NORMAL_METHOD)(Vector3Dd *, SimpleBody *, Vector3Dd *);
typedef void *(*COPY_METHOD)(SimpleBody *);
typedef void (*TRANSLATE_METHOD)(SimpleBody *, Vector3Dd *);
typedef void (*ROTATE_METHOD)(SimpleBody *, Vector3Dd *);
typedef void (*SCALE_METHOD)(SimpleBody *, Vector3Dd *);
typedef void (*INVERT_METHOD)(SimpleBody *);

class Methods {
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

#endif

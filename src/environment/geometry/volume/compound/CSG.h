#ifndef ____
#define ____

#include "app/PovApp.h"
#include "common/FrameConfig.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

class CSG : public Geometry {
  public:
    Geometry *Shapes;

    static int allCsgUnionIntersections(SimpleBody *object,
        RayWithSegments *ray, PriorityQueueNode *depthQueue);
    static int allCsgIntersectIntersections(SimpleBody *object,
        RayWithSegments *ray, PriorityQueueNode *depthQueue);
    static int insideCsgUnion(Vector3Dd *point, SimpleBody *object);
    static int insideCsgIntersection(Vector3Dd *point, SimpleBody *object);
    static void *copyCsg(SimpleBody *object);
    static void translateCsg(SimpleBody *object, Vector3Dd *vector);
    static void rotateCsg(SimpleBody *object, Vector3Dd *vector);
    static void scaleCsg(SimpleBody *object, Vector3Dd *vector);
    static void invertCsg(SimpleBody *object);
};

extern Methods CSG_Union_Methods;
extern Methods CSG_Intersection_Methods;
extern CSG *getCsgShape(void);
extern CSG *getCsgUnion(void);
extern CSG *getCsgIntersection(void);
#endif

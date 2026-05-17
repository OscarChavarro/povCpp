#ifndef ____
#define ____

#include "common/FrameConfig.h"
#include "app/PovApp.h"
#include "common/Vector3D.h"
#include "geom/GeometryOps.h"

class CSG : public Geometry {
  public:
    Geometry *Shapes;

    static int allCsgUnionIntersections(
        SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue);
    static int allCsgIntersectIntersections(
        SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue);
    static int insideCsgUnion(Vector3D *point, SimpleBody *object);
    static int insideCsgIntersection(Vector3D *point, SimpleBody *object);
    static void *copyCsg(SimpleBody *object);
    static void translateCsg(SimpleBody *object, Vector3D *vector);
    static void rotateCsg(SimpleBody *object, Vector3D *vector);
    static void scaleCsg(SimpleBody *object, Vector3D *vector);
    static void invertCsg(SimpleBody *object);
    static void setCsgParents(CSG *shape, SimpleBody *object);
};

extern Methods CSG_Union_Methods;
extern Methods CSG_Intersection_Methods;
extern CSG *getCsgShape(void);
extern CSG *getCsgUnion(void);
extern CSG *getCsgIntersection(void);
#endif

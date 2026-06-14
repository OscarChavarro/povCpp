#ifndef ____
#define ____

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

class CSG : public Geometry {
  public:
    static Methods unionMethodTable;
    static Methods intersectionMethodTable;
    Geometry *Shapes;

    static int allCsgUnionIntersections(SimpleBody *object,
        RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue);
    static int allCsgIntersectIntersections(SimpleBody *object,
        RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue);
    static int insideCsgUnion(Vector3Dd *point, SimpleBody *object);
    static int insideCsgIntersection(Vector3Dd *point, SimpleBody *object);
    static void *copyCsg(SimpleBody *object);
    static void translateCsg(SimpleBody *object, Vector3Dd *vector);
    static void rotateCsg(SimpleBody *object, Vector3Dd *vector);
    static void scaleCsg(SimpleBody *object, Vector3Dd *vector);
    static void invertCsg(SimpleBody *object);

  private:
    static inline void linkShapeNode(
        SimpleBody *newObject, SimpleBody **field, SimpleBody **oldObjectList);
};
#endif

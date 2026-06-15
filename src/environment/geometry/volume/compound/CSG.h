#ifndef ____
#define ____

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"

class CSG : public Geometry {
  public:
    java::ArrayList<Geometry*> Shapes{4};

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int inside(Vector3Dd *point) override;
    void *copy() override;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    void invert() override;

  private:
    int allCsgUnionIntersections(
        RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue);
    int allCsgIntersectIntersections(
        RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue);
    int insideCsgUnion(Vector3Dd *point);
    int insideCsgIntersection(Vector3Dd *point);
};
#endif

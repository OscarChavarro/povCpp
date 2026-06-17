#ifndef ____
#define ____

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/elements/GeometryTypes.h"

class CSG : public Geometry {
  public:
    GeometryTypes geometryType;
    java::ArrayList<TransformableElement*> shapes{4};

    java::ArrayList<TransformableElement*>& getShapes() { return shapes; }

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int allIntersectionsForOwner(
        RayWithSegments *ray,
        java::PriorityQueue<Intersection> *depthQueue,
        SimpleBody *owner) override;
    int inside(Vector3Dd *point) override;
    void *copy() override;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    void invert() override;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;

  private:
    static int insideCsgChild(Vector3Dd *point, TransformableElement *shape);
    int allCsgUnionIntersections(
        RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue);
    int allCsgIntersectIntersections(
        RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue);
    int insideCsgUnion(Vector3Dd *point);
    int insideCsgIntersection(Vector3Dd *point);
};
#endif

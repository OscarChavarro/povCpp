#ifndef ____
#define ____

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/element/GeometryTypes.h"

class CSG : public Geometry {
  public:
    GeometryTypes getGeometryType() const;
    void setGeometryType(GeometryTypes value);
    java::ArrayList<TransformableElement*> &getShapes();
    const java::ArrayList<TransformableElement*> &getShapes() const;

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
    GeometryTypes geometryType;
    java::ArrayList<TransformableElement*> shapes{4};

    static int insideCsgChild(Vector3Dd *point, TransformableElement *shape);
    int allCsgUnionIntersections(
        RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue);
    int allCsgIntersectIntersections(
        RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue);
    int insideCsgUnion(Vector3Dd *point);
    int insideCsgIntersection(Vector3Dd *point);
};

inline GeometryTypes
CSG::getGeometryType() const
{
    return geometryType;
}

inline void
CSG::setGeometryType(GeometryTypes value)
{
    geometryType = value;
}

inline java::ArrayList<TransformableElement*> &
CSG::getShapes()
{
    return shapes;
}

inline const java::ArrayList<TransformableElement*> &
CSG::getShapes() const
{
    return shapes;
}
#endif

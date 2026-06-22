#ifndef __CSG__
#define __CSG__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/volume/compound/BooleanSetOperations.h"

class CSG : public Geometry {
  private:
    BooleanSetOperations geometryType;
    java::ArrayList<TransformableElement*> shapes{4};

    static int insideCsgChild(Vector3Dd *point, TransformableElement *shape);
    int allCsgUnionIntersections(
        RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue);
    int allCsgIntersectIntersections(
        RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue);
    int insideCsgUnion(Vector3Dd *point);
    int insideCsgIntersection(Vector3Dd *point);

  public:
    explicit CSG(BooleanSetOperations initialGeometryType = BooleanSetOperations::UNION);
    CSG(const CSG &other);
    ~CSG() override;

    BooleanSetOperations getGeometryType() const;
    void setGeometryType(BooleanSetOperations value);
    java::ArrayList<TransformableElement*> &getShapes();
    const java::ArrayList<TransformableElement*> &getShapes() const;

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int allIntersectionsForMaterial(
        RayWithSegments *ray,
        java::PriorityQueue<Intersection> *depthQueue,
        Material *material) override;
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
};

inline BooleanSetOperations
CSG::getGeometryType() const
{
    return geometryType;
}

inline void
CSG::setGeometryType(BooleanSetOperations value)
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

#ifndef __CONSTRUCTIVE_SOLID_GEOMETRY__
#define __CONSTRUCTIVE_SOLID_GEOMETRY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/TransformedGeometry.h"
#include "environment/geometry/volume/constructiveSolidGeometry/BooleanSetOperations.h"
#include "environment/scene/SimpleBody.h"

class ConstructiveSolidGeometry : public TransformedGeometry {
  private:
    BooleanSetOperations geometryType;
    java::ArrayList<SimpleBody*> shapes{4};

  public:
    explicit ConstructiveSolidGeometry(BooleanSetOperations initialGeometryType = BooleanSetOperations::UNION);
    ~ConstructiveSolidGeometry() override;

    BooleanSetOperations getGeometryType() const;
    void setGeometryType(BooleanSetOperations value);
    java::ArrayList<SimpleBody*> &getShapes();
    const java::ArrayList<SimpleBody*> &getShapes() const;

    int allIntersectionsForMaterial(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *material) override;
    void translate(Vector3Dd *vector);
    void rotate(Vector3Dd *vector);
    void scale(Vector3Dd *vector);
    void invert();
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue) override = 0;
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override = 0;
    void invertGeometry() override = 0;
};

inline BooleanSetOperations
ConstructiveSolidGeometry::getGeometryType() const
{
    return geometryType;
}

inline void
ConstructiveSolidGeometry::setGeometryType(BooleanSetOperations value)
{
    geometryType = value;
}

inline java::ArrayList<SimpleBody*> &
ConstructiveSolidGeometry::getShapes()
{
    return shapes;
}

inline const java::ArrayList<SimpleBody*> &
ConstructiveSolidGeometry::getShapes() const
{
    return shapes;
}
#endif

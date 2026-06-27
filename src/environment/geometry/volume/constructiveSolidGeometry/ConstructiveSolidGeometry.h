#ifndef __CONSTRUCTIVE_SOLID_GEOMETRY__
#define __CONSTRUCTIVE_SOLID_GEOMETRY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/TransformedGeometry.h"
#include "environment/geometry/volume/constructiveSolidGeometry/BooleanSetOperations.h"

class ConstructiveSolidGeometry : public TransformedGeometry {
  private:
    BooleanSetOperations geometryType;
    java::ArrayList<TransformedGeometry*> shapes{4};
    java::ArrayList<Material*> shapeMaterials{4};

  public:
    explicit ConstructiveSolidGeometry(BooleanSetOperations initialGeometryType = BooleanSetOperations::UNION);
    ~ConstructiveSolidGeometry() override;

    BooleanSetOperations getGeometryType() const;
    void setGeometryType(BooleanSetOperations value);
    void addShape(TransformedGeometry *shape, Material *material);
    java::ArrayList<TransformedGeometry*> &getShapes();
    const java::ArrayList<TransformedGeometry*> &getShapes() const;
    java::ArrayList<Material*> &getShapeMaterials();
    const java::ArrayList<Material*> &getShapeMaterials() const;

    void translate(Vector3Dd *vector);
    void rotate(Vector3Dd *vector);
    void scale(Vector3Dd *vector);
    void invert();
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;

    int doIntersectionForAllRayCrossings(
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) override = 0;
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

inline void
ConstructiveSolidGeometry::addShape(TransformedGeometry *shape, Material *material)
{
    shapes.add(shape);
    shapeMaterials.add(material);
}

inline java::ArrayList<TransformedGeometry*> &
ConstructiveSolidGeometry::getShapes()
{
    return shapes;
}

inline const java::ArrayList<TransformedGeometry*> &
ConstructiveSolidGeometry::getShapes() const
{
    return shapes;
}

inline java::ArrayList<Material*> &
ConstructiveSolidGeometry::getShapeMaterials()
{
    return shapeMaterials;
}

inline const java::ArrayList<Material*> &
ConstructiveSolidGeometry::getShapeMaterials() const
{
    return shapeMaterials;
}
#endif

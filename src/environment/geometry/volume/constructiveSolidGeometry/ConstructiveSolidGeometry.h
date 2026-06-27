#ifndef __CONSTRUCTIVE_SOLID_GEOMETRY__
#define __CONSTRUCTIVE_SOLID_GEOMETRY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/volume/constructiveSolidGeometry/BooleanSetOperations.h"
#include "environment/geometry/volume/constructiveSolidGeometry/CsgOperand.h"

class ConstructiveSolidGeometry : public Geometry {
  private:
    BooleanSetOperations geometryType;
    java::ArrayList<CsgOperand*> operands{4};

  public:
    explicit ConstructiveSolidGeometry(BooleanSetOperations initialGeometryType = BooleanSetOperations::UNION);
    ~ConstructiveSolidGeometry() override;

    BooleanSetOperations getGeometryType() const;
    void setGeometryType(BooleanSetOperations value);
    void addShape(Geometry *shape, Material *material);
    void addOperand(CsgOperand *operand);
    java::ArrayList<CsgOperand*> &getOperands();
    const java::ArrayList<CsgOperand*> &getOperands() const;

    void translate(Vector3Dd *vector);
    void rotate(Vector3Dd *vector);
    void scale(Vector3Dd *vector);
    void invert();

    AxisAlignedBox getMinMax() const override;

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
ConstructiveSolidGeometry::addShape(Geometry *shape, Material *material)
{
    operands.add(new CsgOperand(shape, material));
}

inline void
ConstructiveSolidGeometry::addOperand(CsgOperand *operand)
{
    operands.add(operand);
}

inline java::ArrayList<CsgOperand*> &
ConstructiveSolidGeometry::getOperands()
{
    return operands;
}

inline const java::ArrayList<CsgOperand*> &
ConstructiveSolidGeometry::getOperands() const
{
    return operands;
}
#endif

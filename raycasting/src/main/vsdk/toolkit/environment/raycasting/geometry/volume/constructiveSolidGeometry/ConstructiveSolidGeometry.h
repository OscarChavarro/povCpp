#ifndef __CONSTRUCTIVE_SOLID_GEOMETRY__
#define __CONSTRUCTIVE_SOLID_GEOMETRY__

#include "vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/BooleanSetOperations.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/CsgOperand.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExpression.h"

class ConstructiveSolidGeometry : public RayCastingGeometry, public CsgExpression {
  private:
    BooleanSetOperations geometryType;
    java::ArrayList<CsgOperand*> operands{4};

  public:
    explicit ConstructiveSolidGeometry(BooleanSetOperations initialGeometryType = BooleanSetOperations::UNION);
    ~ConstructiveSolidGeometry() override;

    CsgOperation getOperation() const override { return geometryType; }
    long int getOperandCount() const override { return operands.size(); }
    const CsgOperandView* getOperand(long int index) const override {
        return index < 0 || index >= operands.size() ? nullptr : operands[index];
    }
    BooleanSetOperations getGeometryType() const;
    void setGeometryType(BooleanSetOperations value);
    void addShape(RayCastingGeometry *shape, Material *material);
    void addShape(
        RayCastingGeometry *shape,
        Material *material,
        Matrix4x4d *transformation,
        Matrix4x4d *transformationInverse);
    void addShape(
        RayCastingGeometry *shape,
        Material *material,
        Matrix4x4d *transformation,
        Matrix4x4d *transformationInverse,
        const java::ArrayList<TransformStep> &steps);
    void addOperand(CsgOperand *operand);
    java::ArrayList<CsgOperand*> &getOperands();
    const java::ArrayList<CsgOperand*> &getOperands() const;

    void translate(Vector3Dd *vector);
    void rotate(Vector3Dd *vector);
    void scale(Vector3Dd *vector);
    void invert();

    void getBounds(GeometryBounds& result) const override;
    BoundingVolumeHierarchy *createBoundingVolume() const override;

    int doIntersectionForAllRayCrossings(
        RayWithTracingState *ray,
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
ConstructiveSolidGeometry::addShape(RayCastingGeometry *shape, Material *material)
{
    operands.add(new CsgOperand(shape, material));
}

inline void
ConstructiveSolidGeometry::addShape(
    RayCastingGeometry *shape,
    Material *material,
    Matrix4x4d *transformation,
    Matrix4x4d *transformationInverse)
{
    operands.add(new CsgOperand(shape, material, transformation, transformationInverse));
}

inline void
ConstructiveSolidGeometry::addShape(
    RayCastingGeometry *shape,
    Material *material,
    Matrix4x4d *transformation,
    Matrix4x4d *transformationInverse,
    const java::ArrayList<TransformStep> &steps)
{
    operands.add(new CsgOperand(shape, material, transformation, transformationInverse, steps));
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

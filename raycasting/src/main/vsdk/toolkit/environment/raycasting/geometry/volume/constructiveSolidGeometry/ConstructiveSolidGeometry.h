#ifndef __CONSTRUCTIVE_SOLID_GEOMETRY__
#define __CONSTRUCTIVE_SOLID_GEOMETRY__

#include "vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/BooleanSetOperations.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/CsgOperand.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExpression.h"
#include "vsdk/toolkit/environment/scene/csg/CsgTree.h"
#include "vsdk/toolkit/environment/scene/csg/CsgBodyOperand.h"

/**
A POV CSG geometry: the shared VITRAL `CsgTree` over `CsgOperand` leaves (POV
bodies), which this geometry owns, and one POV evaluation strategy chosen by
the subclass. The operation and operand order are the tree's; the expression
view delegates to it, so VITRAL evaluators and the POV render paths read the
same structure. A nested CSG is a leaf whose geometry is another
`ConstructiveSolidGeometry`, with the leaf's placement.
*/
class ConstructiveSolidGeometry : public RayCastingGeometry, public CsgExpression {
  private:
    CsgTree structure;
    java::ArrayList<CsgOperand*> operands{4};
    java::ArrayList<CsgBodyOperand*> views{4};

  public:
    explicit ConstructiveSolidGeometry(BooleanSetOperations initialGeometryType = BooleanSetOperations::UNION);
    ~ConstructiveSolidGeometry() override;

    const CsgTree &getStructure() const { return structure; }
    CsgOperation getOperation() const override { return structure.getOperation(); }
    long int getOperandCount() const override { return structure.getOperandCount(); }
    const CsgOperandView* getOperand(long int index) const override { return structure.getOperand(index); }
    // The POV evaluators share the Boolean solid of operation and operands;
    // their surface policies are evaluator choices, not a different solid.
    bool isStructuralTraversalSafe() const override;
    bool queryStructureVersion(long long& version) const override { return structure.queryStructureVersion(version); }
    BooleanSetOperations getGeometryType() const { return structure.getOperation(); }
    void setGeometryType(BooleanSetOperations value) { structure.setOperation(value); }
    void addShape(
        RayCastingGeometry *shape,
        Material *material,
        Matrix4x4d *transformation = nullptr,
        Matrix4x4d *transformationInverse = nullptr,
        const java::ArrayList<TransformStep> &steps = java::ArrayList<TransformStep>());
    // Takes ownership of the operand and appends it to the tree.
    void addOperand(CsgOperand *operand);
    const java::ArrayList<CsgOperand*> &getOperands() const { return operands; }

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

#endif

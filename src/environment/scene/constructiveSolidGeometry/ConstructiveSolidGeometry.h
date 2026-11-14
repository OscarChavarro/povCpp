#ifndef __CONSTRUCTIVE_SOLID_GEOMETRY__
#define __CONSTRUCTIVE_SOLID_GEOMETRY__

#include "vsdk/toolkit/environment/scene/csg/CsgOperation.h"
#include "environment/scene/constructiveSolidGeometry/CsgOperand.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExpression.h"
#include "vsdk/toolkit/environment/scene/csg/CsgTree.h"
#include "environment/scene/constructiveSolidGeometry/CsgOperandBodyView.h"
#include "vsdk/toolkit/environment/scene/csg/CsgBodyEvaluator.h"

/**
A POV CSG: the shared VITRAL `CsgTree` over `CsgOperand` leaves (POV bodies),
which this CSG owns, and the evaluator chosen by the parser:
`CROSSING_MEMBERSHIP` runs `CsgMorganRulesEvaluator` (POV-Ray 1.0) and `ROTH`
runs `CsgRaySegmentEvaluator` (`-csgRoth`). The operation and operand order are the tree's; the expression
view delegates to it, so VITRAL evaluators and the POV render paths read the
same structure. A CSG is held by its `PovrayBody` (`PovrayBody::getCsg`), not
by a geometry; a nested CSG is a leaf that holds another one, with the leaf's
placement, and its operand view exposes it as the child expression.
*/
class ConstructiveSolidGeometry : public CsgExpression {
  private:
    CsgTree structure;
    java::ArrayList<CsgOperand*> operands{4};
    java::ArrayList<CsgOperandBodyView*> views{4};
    static void applyAnnotatedEmissionContext(
        IntersectionCandidate &candidate,
        const GeometryIntersectionEmissionContext &context);
    CsgBodyEvaluator evaluator;
    // A Roth CSG that is not an operand of another CSG.
    bool topLevel = false;

  public:
    explicit ConstructiveSolidGeometry(
        CsgOperation initialGeometryType = CsgOperation::UNION,
        CsgBodyEvaluator evaluator = CsgBodyEvaluator::CROSSING_MEMBERSHIP);
    ConstructiveSolidGeometry(const ConstructiveSolidGeometry &other);
    ~ConstructiveSolidGeometry() override;

    CsgBodyEvaluator getEvaluator() const { return evaluator; }
    void setTopLevel(bool value) { topLevel = value; }
    bool isTopLevel() const { return topLevel; }

    CsgOperation getOperation() const override { return structure.getOperation(); }
    long int getOperandCount() const override { return structure.getOperandCount(); }
    const CsgOperandView* getOperand(long int index) const override { return structure.getOperand(index); }
    // The POV evaluators share the Boolean solid of operation and operands;
    // their surface policies are evaluator choices, not a different solid.
    bool isStructuralTraversalSafe() const override { return true; }
    bool queryStructureVersion(long long& version) const override { return structure.queryStructureVersion(version); }
    CsgOperation getGeometryType() const { return structure.getOperation(); }
    void setGeometryType(CsgOperation value) { structure.setOperation(value); }
    void addShape(
        Geometry *shape,
        Material *material,
        Matrix4x4d *transformation = nullptr,
        Matrix4x4d *transformationInverse = nullptr,
        const java::ArrayList<TransformStep> &steps = java::ArrayList<TransformStep>());
    // Appends a leaf that holds the nested CSG `child`.
    void addNested(
        ConstructiveSolidGeometry *child,
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
    // Complements the solid by De Morgan's rules.
    void invert();

    void getBounds(GeometryBounds& result) const;
    BoundingVolumeHierarchy *createBoundingVolume() const;

    int doIntersectionForAllRayCrossings(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr);
    // The crossings with the detail owner and material policy of `context`.
    int doIntersectionForAllRayCrossingsAnnotated(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const GeometryIntersectionEmissionContext &context);
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance);
    ConstructiveSolidGeometry *copy() const;
};

#endif

#ifndef VSDK_CSG_TREE_H
#define VSDK_CSG_TREE_H

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExpression.h"
#include "vsdk/toolkit/environment/scene/csg/CsgOperandView.h"

// An application-built ordered node. Operands are borrowed and must remain
// alive. addOperand rejects cycles visible through the current expression graph.
// A nested tree is identity-placed; evaluators and ownership are separate.
class CsgTree : public CsgExpression, public CsgOperandView {
private:
    CsgOperation operation;
    java::ArrayList<const CsgOperandView*> operands;
    long long structureVersion;
    bool canAttach(const CsgOperandView& operand) const;
    CsgTree(const CsgTree&) = delete;
    CsgTree& operator=(const CsgTree&) = delete;
public:
    explicit CsgTree(CsgOperation operation);
    ~CsgTree() override;
    CsgOperation getOperation() const override;
    long int getOperandCount() const override;
    const CsgOperandView* getOperand(long int index) const override;
    bool addOperand(const CsgOperandView& operand);
    bool isSameSolid(const CsgOperandView& other) const override;
    const Geometry* getGeometry() const override;
    Matrix4x4d getAffinePlacement() const override;
    AffinePlacementStatus queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const override;
    const CsgExpression* getChildExpression() const override;
    bool isStructuralTraversalSafe() const override;
    bool queryStructureVersion(long long& version) const override;
    bool queryModificationVersion(long long& version) const override;
};
#endif

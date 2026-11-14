#include <typeinfo>
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/scene/csg/CsgTree.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExpressionEquivalence.h"

CsgTree::CsgTree(CsgOperation value) : operation(value), operands(4), structureVersion(0) {}
CsgTree::~CsgTree() {}
CsgOperation CsgTree::getOperation() const { return operation; }
long int CsgTree::getOperandCount() const { return operands.size(); }
const CsgOperandView* CsgTree::getOperand(long int index) const
{ return index < 0 || index >= operands.size() ? nullptr : operands[index]; }
bool CsgTree::canAttach(const CsgOperandView& operand) const
{
    const CsgExpression* child = operand.getChildExpression();
    if (!child) return true;
    java::ArrayList<const CsgExpression*> pending;
    java::ArrayList<const CsgExpression*> visited;
    if (!pending.add(child)) return false;
    for (long int i = 0; i < pending.size(); ++i) {
        const CsgExpression* current = pending[i];
        if (current == static_cast<const CsgExpression*>(this)) return false;
        bool known = false;
        for (long int j = 0; j < visited.size(); ++j) {
            if (visited[j] == current) { known = true; break; }
        }
        if (known) continue;
        if (!visited.add(current)) return false;
        for (long int j = 0; j < current->getOperandCount(); ++j) {
            const CsgOperandView* nested = current->getOperand(j);
            if (nested && nested->getChildExpression() && !pending.add(nested->getChildExpression()))
                return false;
        }
    }
    return true;
}
bool CsgTree::addOperand(const CsgOperandView& operand)
{
    if (!canAttach(operand) || !operands.add(&operand)) return false;
    ++structureVersion;
    return true;
}
void CsgTree::setOperation(CsgOperation value)
{
    operation = value;
    ++structureVersion;
}
bool CsgTree::queryStructureVersion(long long& version) const { version=structureVersion; return true; }
bool CsgTree::queryModificationVersion(long long& version) const { version=structureVersion; return true; }
const Geometry* CsgTree::getGeometry() const { return nullptr; }
Matrix4x4d CsgTree::getAffinePlacement() const { return Matrix4x4d::identityMatrix(); }
AffinePlacementStatus CsgTree::queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const
{ forward = Matrix4x4d::identityMatrix(); inverse = forward; return AffinePlacementStatus::Valid; }
const CsgExpression* CsgTree::getChildExpression() const { return this; }

bool CsgTree::isSameSolid(const CsgOperandView& other) const
{ return CsgExpressionEquivalence::same(*this,other); }

bool CsgTree::isStructuralTraversalSafe() const { return typeid(*this)==typeid(CsgTree); }

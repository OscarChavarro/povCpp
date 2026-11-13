#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/scene/csg/CsgVersionStamp.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExpression.h"
#include "vsdk/toolkit/environment/scene/csg/CsgOperandView.h"

CsgVersionStamp::CsgVersionStamp() : expressions(4), expressionVersions(4), operands(8),
    operandVersions(8), path(4), complete(false), captured(false) {}
CsgVersionStamp::CsgVersionStamp(const CsgVersionStamp& other) : expressions(4), expressionVersions(4),
    operands(8), operandVersions(8), path(4), complete(false), captured(false) { *this=other; }
CsgVersionStamp& CsgVersionStamp::operator=(const CsgVersionStamp& other)
{
    if (this!=&other) {
        clear();
        for (long int i=0;i<other.expressions.size();++i) {
            expressions.add(other.expressions[i]); expressionVersions.add(other.expressionVersions[i]);
        }
        for (long int i=0;i<other.operands.size();++i) {
            operands.add(other.operands[i]); operandVersions.add(other.operandVersions[i]);
        }
        complete=other.complete; captured=other.captured;
    }
    return *this;
}
CsgVersionStamp::~CsgVersionStamp() {}
void CsgVersionStamp::clear()
{
    expressions.clear(); expressionVersions.clear(); operands.clear(); operandVersions.clear();
    path.clear(); complete=false; captured=false;
}

bool CsgVersionStamp::record(const CsgExpression& expression)
{
    if (path.contains(&expression) || !path.add(&expression)) return false;
    long long version=0;
    if (!expression.queryStructureVersion(version)) complete=false;
    if (!expressions.add(&expression) || !expressionVersions.add(version)) return false;
    for (long int i=0;i<expression.getOperandCount();++i) {
        const CsgOperandView* operand=expression.getOperand(i);
        if (!operand) { complete=false; continue; }
        long long operandVersion=0;
        if (!operand->queryModificationVersion(operandVersion)) complete=false;
        if (!operands.add(operand) || !operandVersions.add(operandVersion)) return false;
        const CsgExpression* child=operand->getChildExpression();
        if (child && !record(*child)) return false;
    }
    path.remove(path.size()-1);
    return true;
}

bool CsgVersionStamp::capture(const CsgExpression& root)
{
    clear();
    complete=true;
    if (!record(root)) { clear(); return false; }
    path.clear();
    captured=true;
    return true;
}

bool CsgVersionStamp::isCaptured() const { return captured; }
bool CsgVersionStamp::isComplete() const { return captured && complete; }

bool CsgVersionStamp::isCurrent() const
{
    if (!isComplete()) return false;
    for (long int i=0;i<expressions.size();++i) {
        long long version=0;
        if (!expressions[i]->queryStructureVersion(version) || version!=expressionVersions[i]) return false;
    }
    for (long int i=0;i<operands.size();++i) {
        long long version=0;
        if (!operands[i]->queryModificationVersion(version) || version!=operandVersions[i]) return false;
    }
    return true;
}

#ifndef VSDK_CSG_EXPRESSION_EQUIVALENCE_H
#define VSDK_CSG_EXPRESSION_EQUIVALENCE_H
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/scene/csg/CsgOperation.h"
class CsgExpression;
class CsgOperandView;
// Sufficient structural equality; unknown or cyclic expressions are rejected.
class CsgExpressionEquivalence {
private:
    static bool sameView(const CsgOperandView& a, const CsgOperandView& b,
        java::ArrayList<const CsgExpression*>& pathA, java::ArrayList<const CsgExpression*>& pathB);
    static bool collectOperand(const CsgOperandView& view, CsgOperation operation,
        java::ArrayList<const CsgExpression*>& path, java::ArrayList<const CsgOperandView*>& output);
    static bool sameSets(const java::ArrayList<const CsgOperandView*>& a,
        const java::ArrayList<const CsgOperandView*>& b,
        java::ArrayList<const CsgExpression*>& pathA, java::ArrayList<const CsgExpression*>& pathB);
    static bool sameExpression(const CsgExpression& a, const CsgExpression& b,
        java::ArrayList<const CsgExpression*>& pathA, java::ArrayList<const CsgExpression*>& pathB);
public:
    static bool same(const CsgOperandView& a, const CsgOperandView& b);
};
#endif

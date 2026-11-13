#ifndef VSDK_CSG_MEMBERSHIP_RULE_H
#define VSDK_CSG_MEMBERSHIP_RULE_H
#include "vsdk/toolkit/environment/scene/csg/CsgOperation.h"
// Boolean rules shared by every CSG evaluator over an ordered expression.
class CsgMembershipRule {
public:
    // [ROTH1982] Table 3, folded left to right over the operands: the value of
    // operand 0 starts the fold and each later operand is combined with it, so
    // a difference subtracts operands 1..N from operand 0.
    static bool combine(CsgOperation operation, bool accumulated, bool operandInside);
    // POV crossing-membership rule: whether an event of operand eventOperand
    // must lie inside sibling (true) or outside it (false). Union events are
    // never filtered, so it is only meaningful when filtersEvents is true.
    static bool filtersEvents(CsgOperation operation);
    static bool requiresContainment(CsgOperation operation, long int eventOperand, long int sibling);
};

inline bool CsgMembershipRule::combine(CsgOperation operation, bool accumulated, bool operandInside)
{
    if (operation==CsgOperation::UNION) return accumulated || operandInside;
    if (operation==CsgOperation::INTERSECTION) return accumulated && operandInside;
    return accumulated && !operandInside;
}

inline bool CsgMembershipRule::filtersEvents(CsgOperation operation)
{
    return operation!=CsgOperation::UNION;
}

inline bool CsgMembershipRule::requiresContainment(CsgOperation operation, long int eventOperand, long int sibling)
{
    (void)eventOperand;
    return operation==CsgOperation::INTERSECTION || (operation==CsgOperation::DIFFERENCE && sibling==0);
}
#endif

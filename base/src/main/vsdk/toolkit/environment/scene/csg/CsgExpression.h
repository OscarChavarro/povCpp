#ifndef VSDK_CSG_EXPRESSION_H
#define VSDK_CSG_EXPRESSION_H

#include "vsdk/toolkit/environment/scene/csg/CsgOperation.h"
class CsgOperandView;

// An ordered expression view. Difference subtracts operands 1..N from 0.
// Structure and leaf identity are independent of any evaluation strategy.
class CsgExpression {
public:
    virtual ~CsgExpression();
    virtual CsgOperation getOperation() const = 0;
    virtual long int getOperandCount() const = 0;
    virtual const CsgOperandView* getOperand(long int index) const = 0;
};
#endif

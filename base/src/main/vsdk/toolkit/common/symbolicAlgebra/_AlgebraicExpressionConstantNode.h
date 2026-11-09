#ifndef __ALGEBRAIC_EXPRESSION_CONSTANT_NODE__
#define __ALGEBRAIC_EXPRESSION_CONSTANT_NODE__

#include "vsdk/toolkit/common/symbolicAlgebra/_AlgebraicExpressionNode.h"

class _AlgebraicExpressionConstantNode : public _AlgebraicExpressionNode {
private:
    double val;

public:
    explicit _AlgebraicExpressionConstantNode(double val);
    virtual double eval() const override;
    virtual java::String toString() const override;
};

#endif

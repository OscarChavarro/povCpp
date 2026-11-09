#ifndef __ALGEBRAIC_EXPRESSION_NODE__
#define __ALGEBRAIC_EXPRESSION_NODE__

#include "java/lang/String.h"

/**
Node of the expression tree of an `AlgebraicExpression` (composite design
pattern). A node owns its operands.
*/
class _AlgebraicExpressionNode {
public:
    virtual ~_AlgebraicExpressionNode() {}

    /**
    @throws AlgebraicExpressionException if the node can not be evaluated
    */
    virtual double eval() const = 0;
    virtual java::String toString() const = 0;
};

#endif

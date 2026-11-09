#ifndef __ALGEBRAIC_EXPRESSION_UNARY_OPERATOR_NODE__
#define __ALGEBRAIC_EXPRESSION_UNARY_OPERATOR_NODE__

#include "vsdk/toolkit/common/symbolicAlgebra/_AlgebraicExpressionNode.h"

class AlgebraicExpression;

class _AlgebraicExpressionUnaryOperatorNode : public _AlgebraicExpressionNode {
private:
    const AlgebraicExpression* parent;
    java::String operatorName;
    _AlgebraicExpressionNode* operand;

    _AlgebraicExpressionUnaryOperatorNode(
        const _AlgebraicExpressionUnaryOperatorNode& other);
    _AlgebraicExpressionUnaryOperatorNode& operator=(
        const _AlgebraicExpressionUnaryOperatorNode& other);

public:
    _AlgebraicExpressionUnaryOperatorNode(const AlgebraicExpression* parent,
                                          const java::String& operatorName);
    virtual ~_AlgebraicExpressionUnaryOperatorNode();

    /**
    @param operand operand of the operator, owned by this node
    */
    void setOperand(_AlgebraicExpressionNode* operand);
    virtual double eval() const override;
    virtual java::String toString() const override;
};

#endif

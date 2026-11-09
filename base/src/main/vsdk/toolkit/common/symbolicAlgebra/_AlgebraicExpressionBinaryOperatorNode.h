#ifndef __ALGEBRAIC_EXPRESSION_BINARY_OPERATOR_NODE__
#define __ALGEBRAIC_EXPRESSION_BINARY_OPERATOR_NODE__

#include "vsdk/toolkit/common/symbolicAlgebra/_AlgebraicExpressionNode.h"

class AlgebraicExpression;

class _AlgebraicExpressionBinaryOperatorNode : public _AlgebraicExpressionNode {
private:
    const AlgebraicExpression* parent;
    char operatorChar;
    _AlgebraicExpressionNode* leftOperand;
    _AlgebraicExpressionNode* rightOperand;

    _AlgebraicExpressionBinaryOperatorNode(
        const _AlgebraicExpressionBinaryOperatorNode& other);
    _AlgebraicExpressionBinaryOperatorNode& operator=(
        const _AlgebraicExpressionBinaryOperatorNode& other);

public:
    _AlgebraicExpressionBinaryOperatorNode(const AlgebraicExpression* parent,
                                           char op);
    virtual ~_AlgebraicExpressionBinaryOperatorNode();

    /**
    @param operand left operand, owned by this node
    */
    void setLeftOperand(_AlgebraicExpressionNode* operand);

    /**
    @param operand right operand, owned by this node
    */
    void setRightOperand(_AlgebraicExpressionNode* operand);
    virtual double eval() const override;
    virtual java::String toString() const override;
};

#endif

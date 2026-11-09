#ifndef __ALGEBRAIC_EXPRESSION__
#define __ALGEBRAIC_EXPRESSION__

#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "java/util/HashMap.h"

class _AlgebraicExpressionNode;

/**
A `AlgebraicExpression` is an algebraic expression composed of algebraic
variables, unary and binary operators (including basic logarithmic,
exponential, and trigonometric functions) with a set of values for algebraic
variables that can be evaluated, giving as a result a `double` value.

This class establishes a Facade (Facade design pattern for all regular
expression operations) and plays a user role in a composite design pattern
with _AlgebraicExpression*Node classes.

The tokenization (`java.io.StreamTokenizer` as configured by the Java
version) and the construction of the expression tree follow the Java version
exactly, so both ports accept, reject and evaluate the same expressions.
*/
class AlgebraicExpression {
private:
    _AlgebraicExpressionNode* root;
    java::HashMap<java::String, double> values;

    static bool isOperator(const java::String& cad);
    static java::ArrayList<java::String> tokenize(const java::String& text);
    _AlgebraicExpressionNode* buildExpressionTree(
        java::ArrayList<java::String>& tokens) const;

    AlgebraicExpression(const AlgebraicExpression& other);
    AlgebraicExpression& operator=(const AlgebraicExpression& other);

public:
    AlgebraicExpression();
    virtual ~AlgebraicExpression();

    void defineValue(const java::String& name, double val);
    java::String toString() const;

    /**
    @throws AlgebraicExpressionException if the variable is not defined
    */
    double getVariableValue(const java::String& name) const;

    /**
    @param regexp text of the expression
    @throws AlgebraicExpressionException if the expression can not be parsed
    */
    void setExpression(const java::String& regexp);

    /**
    @throws AlgebraicExpressionException if the expression can not be
    evaluated
    */
    double eval() const;
};

#endif

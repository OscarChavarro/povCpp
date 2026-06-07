#ifndef __VSDK_TOOLKIT_COMMON_SYMBOLICALGEBRA_ALGEBRAICEXPRESSION_H__
#define __VSDK_TOOLKIT_COMMON_SYMBOLICALGEBRA_ALGEBRAICEXPRESSION_H__

#include "java/lang/String.h"
#include "java/util/HashMap.h"

class AlgebraicExpression {
private:
    java::String expression;
    java::HashMap<java::String, double> vars;

public:
    void setExpression(const java::String& expr);
    void defineValue(const java::String& name, double value);
    double eval() const;
};

#endif

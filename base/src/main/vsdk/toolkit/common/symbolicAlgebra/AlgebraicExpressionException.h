#ifndef __ALGEBRAIC_EXPRESSION_EXCEPTION__
#define __ALGEBRAIC_EXPRESSION_EXCEPTION__

#include "vsdk/toolkit/common/VSDKException.h"

class AlgebraicExpressionException : public VSDKException {
public:
    AlgebraicExpressionException() : VSDKException() {}
    explicit AlgebraicExpressionException(const java::String& message)
        : VSDKException(message) {}
};

#endif

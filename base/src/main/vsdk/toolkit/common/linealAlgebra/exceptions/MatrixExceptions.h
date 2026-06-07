#ifndef __VSDK_TOOLKIT_COMMON_LINEALALGEBRA_EXCEPTIONS_MATRIXEXCEPTIONS_H__
#define __VSDK_TOOLKIT_COMMON_LINEALALGEBRA_EXCEPTIONS_MATRIXEXCEPTIONS_H__

#include "java/lang/String.h"
#include "vsdk/toolkit/common/VSDKFatalException.h"

class MatrixDimensionMismatchException : public VSDKFatalException {
public:
    explicit MatrixDimensionMismatchException(const java::String& message) : VSDKFatalException(message) {}
};

class MatrixIndexOutOfBoundsException : public VSDKFatalException {
public:
    explicit MatrixIndexOutOfBoundsException(const java::String& message) : VSDKFatalException(message) {}
};

class MatrixNotSquareException : public VSDKFatalException {
public:
    explicit MatrixNotSquareException(const java::String& message) : VSDKFatalException(message) {}
};

class MatrixSingularException : public VSDKFatalException {
public:
    explicit MatrixSingularException(const java::String& message) : VSDKFatalException(message) {}
};

#endif // __VSDK_TOOLKIT_COMMON_LINEALALGEBRA_EXCEPTIONS_MATRIXEXCEPTIONS_H__

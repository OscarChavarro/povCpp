#ifndef __MATRIX_EXCEPTIONS__
#define __MATRIX_EXCEPTIONS__

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

#endif

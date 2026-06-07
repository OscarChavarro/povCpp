#ifndef __VSDK_TOOLKIT_COMMON_VSDK_H__
#define __VSDK_TOOLKIT_COMMON_VSDK_H__


#include "java/lang/String.h"
#include "java/lang/String.h"

class VSDK {
public:
    static const double EPSILON;

    static java::String formatDouble(double a);
    static java::String formatDouble(double a, int digits);
};


#endif // __VSDK_TOOLKIT_COMMON_VSDK_H__

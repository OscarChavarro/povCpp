#ifndef __VSDK_TOOLKIT_COMMON_VSDKFATALEXCEPTION_H__
#define __VSDK_TOOLKIT_COMMON_VSDKFATALEXCEPTION_H__


#include <exception>
#include "java/lang/String.h"

class VSDKFatalException : public std::exception {
private:
    java::String message_;

public:
    explicit VSDKFatalException(const java::String& message) : message_(message) {}

    const char* what() const noexcept override
    {
        return message_.c_str();
    }
};


#endif // __VSDK_TOOLKIT_COMMON_VSDKFATALEXCEPTION_H__

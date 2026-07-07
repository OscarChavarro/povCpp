#ifndef __VSDKFATALEXCEPTION__
#define __VSDKFATALEXCEPTION__


#include "java/lang/String.h"
#include <exception>
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


#endif

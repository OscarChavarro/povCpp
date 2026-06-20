#ifndef JAVA_UTIL_CONCURRENT_CALLABLE_H
#define JAVA_UTIL_CONCURRENT_CALLABLE_H

namespace java {

template<typename T>
class Callable {
public:
    virtual ~Callable() {}
    virtual T call() = 0;
};

}

#endif

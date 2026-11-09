#ifndef __JAVA_CONSUMER__
#define __JAVA_CONSUMER__

#include "java/util/function/_LambdaHolder.h"

namespace java {

/**
Emulation of `java.util.function.Consumer`: a function of one argument returning nothing.
*/
template <class T>
class Consumer : public _Lambda<void, T> {
  public:
    Consumer() : _Lambda<void, T>() {}
    Consumer(decltype(nullptr)) : _Lambda<void, T>() {}
    Consumer(const Consumer &other) : _Lambda<void, T>(static_cast<const _Lambda<void, T> &>(other)) {}

    template <class F>
    Consumer(const F &function) : _Lambda<void, T>(function) {}

    Consumer &
    operator=(const Consumer &other)
    {
        _Lambda<void, T>::operator=(other);
        return *this;
    }

    void
    accept(T value) const
    {
        return this->invoke(value);
    }
};

}

#endif

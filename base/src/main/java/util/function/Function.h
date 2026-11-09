#ifndef __JAVA_FUNCTION__
#define __JAVA_FUNCTION__

#include "java/util/function/_LambdaHolder.h"

namespace java {

/**
Emulation of `java.util.function.Function`: a function of one argument returning a value.
*/
template <class T, class R>
class Function : public _Lambda<R, T> {
  public:
    Function() : _Lambda<R, T>() {}
    Function(decltype(nullptr)) : _Lambda<R, T>() {}
    Function(const Function &other) : _Lambda<R, T>(static_cast<const _Lambda<R, T> &>(other)) {}

    template <class F>
    Function(const F &function) : _Lambda<R, T>(function) {}

    Function &
    operator=(const Function &other)
    {
        _Lambda<R, T>::operator=(other);
        return *this;
    }

    R
    apply(T value) const
    {
        return this->invoke(value);
    }
};

}

#endif

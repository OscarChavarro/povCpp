#ifndef __JAVA_SUPPLIER__
#define __JAVA_SUPPLIER__

#include "java/util/function/_LambdaHolder.h"

namespace java {

/**
Emulation of `java.util.function.Supplier`: a function without arguments returning a value.
*/
template <class T>
class Supplier : public _Lambda<T> {
  public:
    Supplier() : _Lambda<T>() {}
    Supplier(decltype(nullptr)) : _Lambda<T>() {}
    Supplier(const Supplier &other) : _Lambda<T>(static_cast<const _Lambda<T> &>(other)) {}

    template <class F>
    Supplier(const F &function) : _Lambda<T>(function) {}

    Supplier &
    operator=(const Supplier &other)
    {
        _Lambda<T>::operator=(other);
        return *this;
    }

    T
    get() const
    {
        return this->invoke();
    }
};

}

#endif

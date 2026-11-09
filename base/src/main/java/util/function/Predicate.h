#ifndef __JAVA_PREDICATE__
#define __JAVA_PREDICATE__

#include "java/util/function/_LambdaHolder.h"

namespace java {

/**
Emulation of `java.util.function.Predicate`: a boolean valued function of one argument.
*/
template <class T>
class Predicate : public _Lambda<bool, T> {
  public:
    Predicate() : _Lambda<bool, T>() {}
    Predicate(decltype(nullptr)) : _Lambda<bool, T>() {}
    Predicate(const Predicate &other) : _Lambda<bool, T>(static_cast<const _Lambda<bool, T> &>(other)) {}

    template <class F>
    Predicate(const F &function) : _Lambda<bool, T>(function) {}

    Predicate &
    operator=(const Predicate &other)
    {
        _Lambda<bool, T>::operator=(other);
        return *this;
    }

    bool
    test(T value) const
    {
        return this->invoke(value);
    }
};

}

#endif

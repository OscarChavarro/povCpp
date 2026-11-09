#ifndef __JAVA_BI_CONSUMER__
#define __JAVA_BI_CONSUMER__

#include "java/util/function/_LambdaHolder.h"

namespace java {

/**
Emulation of `java.util.function.BiConsumer`: a function of two arguments returning nothing.
*/
template <class T, class U>
class BiConsumer : public _Lambda<void, T, U> {
  public:
    BiConsumer() : _Lambda<void, T, U>() {}
    BiConsumer(decltype(nullptr)) : _Lambda<void, T, U>() {}
    BiConsumer(const BiConsumer &other) : _Lambda<void, T, U>(static_cast<const _Lambda<void, T, U> &>(other)) {}

    template <class F>
    BiConsumer(const F &function) : _Lambda<void, T, U>(function) {}

    BiConsumer &
    operator=(const BiConsumer &other)
    {
        _Lambda<void, T, U>::operator=(other);
        return *this;
    }

    void
    accept(T first, U second) const
    {
        return this->invoke(first, second);
    }
};

}

#endif

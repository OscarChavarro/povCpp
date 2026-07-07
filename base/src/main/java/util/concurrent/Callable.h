#ifndef __CALLABLE__
#define __CALLABLE__

namespace java {

template<typename T>
class Callable {
public:
    virtual ~Callable() {}
    virtual T call() = 0;
};

}

#endif

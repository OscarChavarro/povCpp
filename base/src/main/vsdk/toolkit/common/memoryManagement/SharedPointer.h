#ifndef __SHARED_POINTER__
#define __SHARED_POINTER__

#include "java/util/concurrent/atomic/AtomicLong.h"

/**
Reference counted pointer (the Java references to an object shared by
several owners): the pointed object is deleted when the last
`SharedPointer` referencing it is destroyed or reset. The reference counter
is thread safe, but (as for any object) one `SharedPointer` must not be
modified by one thread while other threads access it.
*/
template<class T>
class SharedPointer {
  private:
    T *pointer;
    java::AtomicLong *references;
    /// False for the pointers to objects owned by someone else
    bool owning;

    void
    retain()
    {
        if ( references != nullptr ) {
            references->incrementAndGet();
        }
    }

    void
    release()
    {
        if ( references != nullptr && references->decrementAndGet() == 0 ) {
            if ( owning ) {
                delete pointer;
            }
            delete references;
        }
        pointer = nullptr;
        references = nullptr;
    }

  public:
    SharedPointer() : pointer(nullptr), references(nullptr), owning(true) {}

    explicit SharedPointer(T *inPointer) :
        pointer(inPointer),
        references(inPointer != nullptr ? new java::AtomicLong(1) : nullptr),
        owning(true)
    {
    }

    SharedPointer(const SharedPointer<T> &other) :
        pointer(other.pointer), references(other.references),
        owning(other.owning)
    {
        retain();
    }

    /**
    @return a pointer that never deletes its object, owned by someone else
    that must outlive every copy of the pointer
    */
    static SharedPointer<T>
    notOwning(T *inPointer)
    {
        SharedPointer<T> result(inPointer);
        result.owning = false;
        return result;
    }

    ~SharedPointer()
    {
        release();
    }

    SharedPointer<T> &
    operator=(const SharedPointer<T> &other)
    {
        if ( references != other.references ) {
            SharedPointer<T> copy(other);
            release();
            pointer = copy.pointer;
            references = copy.references;
            owning = copy.owning;
            copy.pointer = nullptr;
            copy.references = nullptr;
        }
        return *this;
    }

    void
    reset()
    {
        release();
    }

    T *
    get() const
    {
        return pointer;
    }

    T *
    operator->() const
    {
        return pointer;
    }

    T &
    operator*() const
    {
        return *pointer;
    }

    explicit operator bool() const
    {
        return pointer != nullptr;
    }
};

#endif

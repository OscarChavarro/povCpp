#ifndef __CONCURRENTLINKEDQUEUE__
#define __CONCURRENTLINKEDQUEUE__

#include "java/util/ArrayDeque.h"
#include <pthread.h>
namespace java {

template<typename T>
class ConcurrentLinkedQueue {
private:
    ArrayDeque<T> data_;
    mutable pthread_mutex_t mutex_;

public:
    ConcurrentLinkedQueue() { pthread_mutex_init(&mutex_, 0); }

    template<typename Container>
    explicit ConcurrentLinkedQueue(Container& initial)
    {
        pthread_mutex_init(&mutex_, 0);
        for (long int i = 0; i < (long int)initial.size(); ++i) {
            data_.addLast(initial[i]);
        }
    }

    ~ConcurrentLinkedQueue() { pthread_mutex_destroy(&mutex_); }

    void add(const T& item)
    {
        pthread_mutex_lock(&mutex_);
        data_.addLast(item);
        pthread_mutex_unlock(&mutex_);
    }

    bool poll(T* out)
    {
        if ( out == 0 ) return false;
        pthread_mutex_lock(&mutex_);
        if ( data_.isEmpty() ) {
            pthread_mutex_unlock(&mutex_);
            return false;
        }
        *out = data_.peekFirst();
        data_.removeFirst();
        pthread_mutex_unlock(&mutex_);
        return true;
    }

    bool isEmpty() const
    {
        pthread_mutex_lock(&mutex_);
        bool empty = data_.isEmpty();
        pthread_mutex_unlock(&mutex_);
        return empty;
    }
};

}

#endif

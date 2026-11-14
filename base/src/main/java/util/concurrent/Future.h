#ifndef __FUTURE__
#define __FUTURE__

#include "java/lang/String.h"
#include "java/util/concurrent/ExecutionException.h"
#include "java/util/concurrent/atomic/AtomicLong.h"
#include <pthread.h>
namespace java {

/**
Emulation of `java.util.concurrent.Future`: copies of a future share one
result state, as Java references to the same future do. The state counts its
references (the futures and the pending task that completes it) and is deleted
by the last one released.
*/
template<typename T>
class Future {
public:
    class SharedState {
    private:
        AtomicLong references;

    public:
        pthread_mutex_t mutex;
        pthread_cond_t cond;
        bool done;
        bool failed;
        java::String errorMessage;
        T value;

        /// Starts with one reference, owned by its creator.
        SharedState() : references(1), done(false), failed(false), errorMessage(), value() {
            pthread_mutex_init(&mutex, 0);
            pthread_cond_init(&cond, 0);
        }

        SharedState(const SharedState&) = delete;
        SharedState& operator=(const SharedState&) = delete;

        ~SharedState() {
            pthread_cond_destroy(&cond);
            pthread_mutex_destroy(&mutex);
        }

        void retain() { references.incrementAndGet(); }

        void release()
        {
            if ( references.decrementAndGet() == 0 ) {
                delete this;
            }
        }
    };

private:
    SharedState* state_;

public:
    Future() : state_(new SharedState()) {}

    /// Shares the given state; the caller keeps its own reference.
    explicit Future(SharedState* state) : state_(state) { state_->retain(); }

    Future(const Future<T>& other) : state_(other.state_) { state_->retain(); }

    Future<T>& operator=(const Future<T>& other)
    {
        if ( state_ != other.state_ ) {
            other.state_->retain();
            state_->release();
            state_ = other.state_;
        }
        return *this;
    }

    ~Future() { state_->release(); }

    T get()
    {
        pthread_mutex_lock(&state_->mutex);
        while ( !state_->done ) {
            pthread_cond_wait(&state_->cond, &state_->mutex);
        }
        bool failed = state_->failed;
        java::String message = state_->errorMessage;
        T value = state_->value;
        pthread_mutex_unlock(&state_->mutex);

        if ( failed ) {
            throw ExecutionException(message.isEmpty() ? java::String("Task execution failed") : message);
        }
        return value;
    }
};

}

#endif

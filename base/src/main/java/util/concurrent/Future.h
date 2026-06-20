#ifndef JAVA_UTIL_CONCURRENT_FUTURE_H
#define JAVA_UTIL_CONCURRENT_FUTURE_H

#include "java/lang/String.h"
#include "java/util/concurrent/ExecutionException.h"
#include <pthread.h>
#include <memory>
namespace java {

template<typename T>
class Future {
public:
    struct SharedState {
        pthread_mutex_t mutex;
        pthread_cond_t cond;
        bool done;
        bool failed;
        java::String errorMessage;
        T value;

        SharedState() : done(false), failed(false), errorMessage(), value() {
            pthread_mutex_init(&mutex, 0);
            pthread_cond_init(&cond, 0);
        }

        ~SharedState() {
            pthread_cond_destroy(&cond);
            pthread_mutex_destroy(&mutex);
        }
    };

private:
    std::shared_ptr<SharedState> state_;

public:
    Future() : state_(new SharedState()) {}
    explicit Future(const std::shared_ptr<SharedState>& state) : state_(state) {}

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
            throw ExecutionException(message.empty() ? java::String("Task execution failed") : message);
        }
        return value;
    }
};

}

#endif

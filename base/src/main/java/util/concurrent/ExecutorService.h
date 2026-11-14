#ifndef __EXECUTOR_SERVICE__
#define __EXECUTOR_SERVICE__

#include "java/lang/Exception.h"
#include "java/util/concurrent/Callable.h"
#include "java/util/concurrent/Future.h"
namespace java {

class ExecutorService {
protected:
    class TaskBase {
    public:
        virtual ~TaskBase() {}
        virtual void run() = 0;
    };

    virtual bool enqueue(TaskBase* task) = 0;

public:
    virtual ~ExecutorService() {}
    virtual void shutdownNow() = 0;

    template<typename T>
    Future<T> submit(Callable<T>* callable)
    {
        // submit() owns one reference until the future is built; the task
        // owns another, and the callable, until it is deleted.
        typename Future<T>::SharedState* state = new typename Future<T>::SharedState();

        class TaskImpl : public TaskBase {
        private:
            Callable<T>* callable_;
            typename Future<T>::SharedState* state_;
        public:
            TaskImpl(Callable<T>* callable, typename Future<T>::SharedState* state)
                : callable_(callable), state_(state) { state_->retain(); }

            virtual ~TaskImpl() { delete callable_; state_->release(); }

            virtual void run()
            {
                try {
                    T value = callable_->call();
                    pthread_mutex_lock(&state_->mutex);
                    state_->value = value;
                    state_->done = true;
                    pthread_cond_broadcast(&state_->cond);
                    pthread_mutex_unlock(&state_->mutex);
                }
                catch (const java::Exception& e) {
                    pthread_mutex_lock(&state_->mutex);
                    state_->failed = true;
                    state_->errorMessage = e.getMessage().c_str();
                    state_->done = true;
                    pthread_cond_broadcast(&state_->cond);
                    pthread_mutex_unlock(&state_->mutex);
                }
                catch (...) {
                    pthread_mutex_lock(&state_->mutex);
                    state_->failed = true;
                    state_->errorMessage = "unknown exception";
                    state_->done = true;
                    pthread_cond_broadcast(&state_->cond);
                    pthread_mutex_unlock(&state_->mutex);
                }
                delete callable_;
                callable_ = nullptr;
            }
        };

        TaskBase* task = new TaskImpl(callable, state);
        if ( !enqueue(task) ) {
            pthread_mutex_lock(&state->mutex);
            state->failed = true;
            state->errorMessage = "executor rejected task";
            state->done = true;
            pthread_cond_broadcast(&state->cond);
            pthread_mutex_unlock(&state->mutex);
            delete task;
        }

        Future<T> result(state);
        state->release();
        return result;
    }
};

}

#endif

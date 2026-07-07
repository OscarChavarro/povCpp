#ifndef __EXECUTORSERVICE__
#define __EXECUTORSERVICE__

#include "java/util/concurrent/Callable.h"
#include "java/util/concurrent/Future.h"
#include <exception>
#include <memory>
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
        std::shared_ptr<typename Future<T>::SharedState> state(new typename Future<T>::SharedState());

        class TaskImpl : public TaskBase {
        private:
            Callable<T>* callable_;
            std::shared_ptr<typename Future<T>::SharedState> state_;
        public:
            TaskImpl(Callable<T>* callable, const std::shared_ptr<typename Future<T>::SharedState>& state)
                : callable_(callable), state_(state) {}

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
                catch (const std::exception& e) {
                    pthread_mutex_lock(&state_->mutex);
                    state_->failed = true;
                    state_->errorMessage = e.what();
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

        return Future<T>(state);
    }
};

}

#endif

#include "java/util/ArrayDeque.h"
#include "java/util/ArrayList.txx"
#include "java/util/concurrent/Executors.h"
#include <pthread.h>
namespace java {

class FixedThreadPoolExecutor : public ExecutorService {
private:
    struct WorkerContext {
        FixedThreadPoolExecutor* owner;
    };

    ArrayDeque<TaskBase*> queue_;
    pthread_mutex_t mutex_;
    pthread_cond_t hasWorkCond_;
    bool shutdown_;

#ifdef VITRAL_WITH_POSIX_THREADS
    ArrayList<pthread_t> workers_;
#endif

    static void* workerMain(void* arg)
    {
        WorkerContext* ctx = reinterpret_cast<WorkerContext*>(arg);
        FixedThreadPoolExecutor* self = ctx->owner;
        delete ctx;

        while ( true ) {
            TaskBase* task = 0;
            pthread_mutex_lock(&self->mutex_);
            while ( self->queue_.isEmpty() && !self->shutdown_ ) {
                pthread_cond_wait(&self->hasWorkCond_, &self->mutex_);
            }

            if ( self->shutdown_ && self->queue_.isEmpty() ) {
                pthread_mutex_unlock(&self->mutex_);
                break;
            }

            task = self->queue_.peekFirst();
            self->queue_.removeFirst();
            pthread_mutex_unlock(&self->mutex_);

            if ( task != 0 ) {
                task->run();
                delete task;
            }
        }
        return 0;
    }

public:
    explicit FixedThreadPoolExecutor(int numberOfThreads)
        : queue_(), shutdown_(false)
    {
        pthread_mutex_init(&mutex_, 0);
        pthread_cond_init(&hasWorkCond_, 0);

#ifdef VITRAL_WITH_POSIX_THREADS
        if ( numberOfThreads < 1 ) {
            numberOfThreads = 1;
        }
        for (int i = 0; i < numberOfThreads; ++i) {
            workers_.add((pthread_t)0);
        }
        for (int i = 0; i < numberOfThreads; ++i) {
            WorkerContext* ctx = new WorkerContext();
            ctx->owner = this;
            pthread_create(&workers_[i], 0, &FixedThreadPoolExecutor::workerMain, ctx);
        }
#endif
    }

    virtual ~FixedThreadPoolExecutor()
    {
        shutdownNow();
        pthread_cond_destroy(&hasWorkCond_);
        pthread_mutex_destroy(&mutex_);
    }

    virtual void shutdownNow()
    {
        pthread_mutex_lock(&mutex_);
        if ( shutdown_ ) {
            pthread_mutex_unlock(&mutex_);
            return;
        }
        shutdown_ = true;
        pthread_cond_broadcast(&hasWorkCond_);
        pthread_mutex_unlock(&mutex_);

#ifdef VITRAL_WITH_POSIX_THREADS
        for (long int i = 0; i < workers_.size(); ++i) {
            pthread_join(workers_[i], 0);
        }
        workers_.clear();
#endif

        pthread_mutex_lock(&mutex_);
        while ( !queue_.isEmpty() ) {
            TaskBase* task = queue_.peekFirst();
            queue_.removeFirst();
            delete task;
        }
        pthread_mutex_unlock(&mutex_);
    }

protected:
    virtual bool enqueue(TaskBase* task)
    {
        pthread_mutex_lock(&mutex_);
        if ( shutdown_ ) {
            pthread_mutex_unlock(&mutex_);
            return false;
        }
#ifdef VITRAL_WITH_POSIX_THREADS
        queue_.addLast(task);
        pthread_cond_signal(&hasWorkCond_);
        pthread_mutex_unlock(&mutex_);
#else
        pthread_mutex_unlock(&mutex_);
        task->run();
        delete task;
#endif
        return true;
    }
};

ExecutorService* Executors::newFixedThreadPool(int numberOfThreads)
{
    return new FixedThreadPoolExecutor(numberOfThreads);
}

}

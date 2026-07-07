#ifndef __EXECUTORS__
#define __EXECUTORS__

#include "java/util/concurrent/ExecutorService.h"
namespace java {

class Executors {
public:
    static ExecutorService* newFixedThreadPool(int numberOfThreads);
};

}

#endif

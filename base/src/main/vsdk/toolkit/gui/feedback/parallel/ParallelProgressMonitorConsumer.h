#ifndef __PARALLEL_PROGRESS_MONITOR_CONSUMER__
#define __PARALLEL_PROGRESS_MONITOR_CONSUMER__

#include "java/util/concurrent/ConcurrentLinkedQueue.h"
#include "vsdk/toolkit/gui/feedback/ProgressMonitorConsoleLongFormat.h"
#include "vsdk/toolkit/gui/feedback/parallel/ParallelProgressMonitorEvent.h"
class ParallelProgressMonitorConsumer {
private:
    ProgressMonitorConsoleLongFormat concreteProgressMonitor;
    java::ConcurrentLinkedQueue<ParallelProgressMonitorEvent>* sharedEventQueue;
    bool stillProcessingEvents;
    long long totalElementsToProcess;
    long long currentProcessedElements;

public:
    explicit ParallelProgressMonitorConsumer(
        java::ConcurrentLinkedQueue<ParallelProgressMonitorEvent>* sharedEventQueue);
    virtual ~ParallelProgressMonitorConsumer() {}

    void run();
};

#endif

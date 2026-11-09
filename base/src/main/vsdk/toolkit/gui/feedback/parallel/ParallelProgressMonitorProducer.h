#ifndef __PARALLEL_PROGRESS_MONITOR_PRODUCER__
#define __PARALLEL_PROGRESS_MONITOR_PRODUCER__

#include "java/util/concurrent/ConcurrentLinkedQueue.h"
#include "java/util/concurrent/atomic/AtomicLong.h"
#include "vsdk/toolkit/gui/feedback/ProgressMonitor.h"
#include "vsdk/toolkit/gui/feedback/parallel/ParallelProgressMonitorEvent.h"
class ParallelProgressMonitorProducer : public ProgressMonitor {
private:
    java::ConcurrentLinkedQueue<ParallelProgressMonitorEvent>* sharedEventQueue;
    java::AtomicLong processedElements;
    volatile long long totalElements;

public:
    explicit ParallelProgressMonitorProducer(
        java::ConcurrentLinkedQueue<ParallelProgressMonitorEvent>* sharedEventQueue);
    virtual ~ParallelProgressMonitorProducer() override {}

    void init(long long totalElementsToProcess);
    void finish();

    virtual void begin() override;
    virtual void end() override;
    virtual void update(double minimumValue, double maximumValue, double currentValue) override;
    virtual double getCurrentPercent() override;
};

#endif

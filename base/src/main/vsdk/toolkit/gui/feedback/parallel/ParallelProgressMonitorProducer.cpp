#include "vsdk/toolkit/gui/feedback/parallel/ParallelProgressMonitorProducer.h"
ParallelProgressMonitorProducer::ParallelProgressMonitorProducer(
    java::ConcurrentLinkedQueue<ParallelProgressMonitorEvent>* queue)
    : sharedEventQueue(queue), processedElements(0), totalElements(0)
{
}

void ParallelProgressMonitorProducer::init(long long totalElementsToProcess)
{
    totalElements = totalElementsToProcess;
    processedElements.set(0);
    sharedEventQueue->add(ParallelProgressMonitorEvent(INIT, totalElementsToProcess));
}

void ParallelProgressMonitorProducer::finish()
{
    sharedEventQueue->add(ParallelProgressMonitorEvent(FINISH, 0));
}

void ParallelProgressMonitorProducer::begin()
{
}

void ParallelProgressMonitorProducer::end()
{
}

void ParallelProgressMonitorProducer::update(double, double, double)
{
    processedElements.incrementAndGet();
    sharedEventQueue->add(ParallelProgressMonitorEvent(PROCESS_NEXT_ELEMENT, 0));
}

double ParallelProgressMonitorProducer::getCurrentPercent()
{
    long long total = totalElements;
    if ( total <= 0 ) {
        return 0;
    }
    return 100.0 * static_cast<double>(processedElements.get()) / static_cast<double>(total);
}

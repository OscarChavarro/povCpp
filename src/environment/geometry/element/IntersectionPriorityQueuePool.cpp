#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "environment/geometry/element/Intersection.h"
#include "environment/geometry/element/IntersectionPriorityQueuePool.h"

IntersectionPriorityQueuePool::IntersectionPriorityQueuePool()
    : queues(nullptr), nextFreeIndex{}, headIndex(-1)
{
}

IntersectionPriorityQueuePool::~IntersectionPriorityQueuePool()
{
    if (queues == nullptr) {
        return;
    }

    for (int i = 0; i < NUMBER_OF_PRIOQS; i++) {
        queues[i].~PriorityQueue<Intersection>();
    }
    ::operator delete[](queues);
}

void
IntersectionPriorityQueuePool::init()
{
    if (queues == nullptr) {
        void * const rawStorage =
            ::operator new[](sizeof(java::PriorityQueue<Intersection>) * NUMBER_OF_PRIOQS);
        queues = static_cast<java::PriorityQueue<Intersection> *>(rawStorage);
        if (queues == nullptr) {
            Logger::reportMessage("IntersectionPriorityQueuePool", Logger::FATAL_ERROR, "", "\nOut of memory. Cannot allocate queue storage");
        }

        for (int i = 0; i < NUMBER_OF_PRIOQS; i++) {
            new (queues + i) java::PriorityQueue<Intersection>(MAX_NUMBER_OF_ENTRIES);
        }
    }

    for (int i = 0; i < NUMBER_OF_PRIOQS; i++) {
        queues[i].clear();
        java::PriorityQueueAccess<Intersection>::setActiveLimit(queues[i], -1);
        nextFreeIndex[i] = i + 1;
    }
    nextFreeIndex[NUMBER_OF_PRIOQS - 1] = -1;
    headIndex = 0;
}

java::PriorityQueue<Intersection> *
IntersectionPriorityQueuePool::pop(int indexSize)
{
    static constexpr int MAX_NUMBER_OF_ENTRIES = 128;
    if (indexSize >= MAX_NUMBER_OF_ENTRIES) {
        indexSize = MAX_NUMBER_OF_ENTRIES - 1;
    }
    if (queues == nullptr) {
        init();
    }
    if (headIndex < 0) {
        Logger::reportMessage("IntersectionPriorityQueuePool", Logger::FATAL_ERROR, "", "\nOut of prioqs");
    }

    const int index = headIndex;
    java::PriorityQueue<Intersection> * const pq = queues + index;
    headIndex = nextFreeIndex[index];
    pq->clear();
    java::PriorityQueueAccess<Intersection>::setActiveLimit(*pq, indexSize);
    return pq;
}

void
IntersectionPriorityQueuePool::push(java::PriorityQueue<Intersection> *queue)
{
    if (queues == nullptr || queue < queues || queue >= queues + NUMBER_OF_PRIOQS) {
        Logger::reportMessage("IntersectionPriorityQueuePool", Logger::FATAL_ERROR, "", "\nInvalid prioq");
    }

    const int index = static_cast<int>(queue - queues);
    queue->clear();
    java::PriorityQueueAccess<Intersection>::setActiveLimit(*queue, -1);
    nextFreeIndex[index] = headIndex;
    headIndex = index;
}

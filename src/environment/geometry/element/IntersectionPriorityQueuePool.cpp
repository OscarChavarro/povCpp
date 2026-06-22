#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.txx"
#include "environment/geometry/element/IntersectionCandidate.h"
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
        queues[i].~PriorityQueue<IntersectionCandidate>();
    }
}

void
IntersectionPriorityQueuePool::init()
{
    if (queues == nullptr) {
        storage.init(sizeof(java::PriorityQueue<IntersectionCandidate>) * NUMBER_OF_PRIOQS);
        queues = storage.allocate(NUMBER_OF_PRIOQS);
        if (queues == nullptr) {
            Logger::reportMessage("IntersectionPriorityQueuePool", Logger::FATAL_ERROR, "", "\nOut of memory. Cannot allocate queue storage");
        }

        for (int i = 0; i < NUMBER_OF_PRIOQS; i++) {
            new (queues + i) java::PriorityQueue<IntersectionCandidate>(MAX_NUMBER_OF_ENTRIES);
        }
    }

    for (int i = 0; i < NUMBER_OF_PRIOQS; i++) {
        queues[i].clear();
        java::PriorityQueueAccess<IntersectionCandidate>::setActiveLimit(queues[i], -1);
        nextFreeIndex[i] = i + 1;
    }
    nextFreeIndex[NUMBER_OF_PRIOQS - 1] = -1;
    headIndex = 0;
}

java::PriorityQueue<IntersectionCandidate> *
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
    java::PriorityQueue<IntersectionCandidate> * const pq = queues + index;
    headIndex = nextFreeIndex[index];
    pq->clear();
    java::PriorityQueueAccess<IntersectionCandidate>::setActiveLimit(*pq, indexSize);
    return pq;
}

void
IntersectionPriorityQueuePool::push(java::PriorityQueue<IntersectionCandidate> *queue)
{
    if (queues == nullptr || queue < queues || queue >= queues + NUMBER_OF_PRIOQS) {
        Logger::reportMessage("IntersectionPriorityQueuePool", Logger::FATAL_ERROR, "", "\nInvalid prioq");
    }

    const int index = static_cast<int>(queue - queues);
    queue->clear();
    java::PriorityQueueAccess<IntersectionCandidate>::setActiveLimit(*queue, -1);
    nextFreeIndex[index] = headIndex;
    headIndex = index;
}

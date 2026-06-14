#ifndef PriorityQueue__
#define PriorityQueue__

#include "java/lang/Object.h"

namespace java {

template <class T>
class PriorityQueueAccess;

template <class T>
class PriorityQueue final : public Object {
  private:
    static const int DEFAULT_CAPACITY = 11;

    int currentSize;
    int maxSize;
    int activeLimit;
    T *data;

    void init(int initialCapacity);
    void allocateStorage(int capacity);
    void releaseStorage();
    void ensureCapacity(int requiredCapacity);
    void siftUp(int index);
    void siftDown(int index);
    bool lessThan(const T& a, const T& b) const;

    friend class PriorityQueueAccess<T>;

  public:
    PriorityQueue();
    PriorityQueue(int initialCapacity);
    virtual ~PriorityQueue();

    bool add(const T& elem);
    bool offer(const T& elem);
    T peek();
    T poll();
    int size() const;
    void clear();
};

template <class T>
class PriorityQueueAccess final {
  public:
    static inline void setActiveLimit(PriorityQueue<T> &queue, int limit)
    {
        queue.activeLimit = limit;
    }
};

}

#endif

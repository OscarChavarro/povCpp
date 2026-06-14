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
    // Iterator: JDK-style unordered traversal of the internal heap array [1..currentSize]
    class Iterator {
      private:
        const PriorityQueue<T>* owner;
        int cursor;

        friend class PriorityQueue<T>;
        Iterator(const PriorityQueue<T>* q, int start) : owner(q), cursor(start) {}

      public:
        bool hasNext() const
        {
            return cursor <= owner->currentSize;
        }

        T next()
        {
            return owner->data[cursor++];
        }
    };

    PriorityQueue();
    PriorityQueue(int initialCapacity);
    virtual ~PriorityQueue();

    bool add(const T& elem);
    bool offer(const T& elem);
    T peek();
    T poll();
    int size() const;
    void clear();

    Iterator iterator() const;

    template <class Predicate> bool removeIf(Predicate p);
    template <class Consumer> void forEach(Consumer action) const;

    bool contains(const T& elem) const;
    bool remove(const T& elem);
    T* toArray() const;
    T* toArray(T* target) const;

    // C++ ergonomic helpers for range-for loops (not in JDK API; expose iterator to C++ idioms)
    const T* begin() const;
    const T* end() const;
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

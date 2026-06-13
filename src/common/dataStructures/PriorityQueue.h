#ifndef __PRIOQ_H__
#define __PRIOQ_H__

template <class T>
class PriorityQueue {
  private:
    static const int DEFAULT_CAPACITY = 11;

    int currentSize;
    int maxSize;
    int storageCapacity;
    T *data;
    PriorityQueue<T> *nextQueue;

    void init(int initialCapacity);
    void allocateStorage(int capacity);
    void releaseStorage();
    void reset();
    void siftUp(int index);
    void siftDown(int index);
    bool lessThan(const T& a, const T& b) const;

  public:
    PriorityQueue();
    explicit PriorityQueue(int initialCapacity);
    ~PriorityQueue();

    bool add(const T& elem);
    bool offer(const T& elem);
    T peek() const;
    T poll();
    int size() const;
    void clear();
    inline PriorityQueue<T> *poolNext() const;
    inline void setPoolNext(PriorityQueue<T> *next);
    inline void resetForPool(int limit);
};

#endif

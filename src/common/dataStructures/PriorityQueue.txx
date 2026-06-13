#ifndef __PRIOQ_TXX__
#define __PRIOQ_TXX__

#include "common/dataStructures/PriorityQueue.h"

template <class T>
PriorityQueue<T>::PriorityQueue()
{
    init(DEFAULT_CAPACITY);
}

template <class T>
PriorityQueue<T>::PriorityQueue(int initialCapacity)
{
    init(initialCapacity);
}

template <class T>
PriorityQueue<T>::~PriorityQueue()
{
    releaseStorage();
}

template <class T>
void
PriorityQueue<T>::init(int initialCapacity)
{
    currentSize = 0;
    maxSize = 0;
    storageCapacity = 0;
    data = nullptr;
    nextQueue = nullptr;
    allocateStorage(initialCapacity);
    maxSize = storageCapacity;
}

template <class T>
void
PriorityQueue<T>::allocateStorage(int capacity)
{
    if (capacity < 2) {
        capacity = 2;
    }
    data = new T[capacity];
    storageCapacity = capacity;
}

template <class T>
void
PriorityQueue<T>::releaseStorage()
{
    delete[] data;
    data = nullptr;
    storageCapacity = 0;
    maxSize = 0;
    currentSize = 0;
}

template <class T>
void
PriorityQueue<T>::reset()
{
    currentSize = 0;
}

template <class T>
bool
PriorityQueue<T>::lessThan(const T& a, const T& b) const
{
    return a.Depth < b.Depth;
}

template <class T>
void
PriorityQueue<T>::siftDown(int index)
{
    int childIndex;
    T *entry;
    T *child;
    T tempEntry;

    entry = &data[index];
    childIndex = index * 2;

    if ((childIndex < maxSize) && (childIndex <= currentSize)) {
        if ((childIndex + 1 > currentSize) ||
            lessThan(data[childIndex], data[childIndex + 1])) {
            child = &data[childIndex];
        } else {
            childIndex++;
            child = &data[childIndex];
        }

        if (lessThan(*child, *entry)) {
            tempEntry = *entry;
            *entry = *child;
            *child = tempEntry;
            siftDown(childIndex);
        }
    }
}

template <class T>
void
PriorityQueue<T>::siftUp(int index)
{
    int parentIndex;
    T *entry;
    T *parent;
    T tempEntry;

    if (index / 2 < 1) {
        return;
    }

    entry = &data[index];
    parentIndex = index / 2;
    parent = &data[parentIndex];
    if (lessThan(*entry, *parent)) {
        tempEntry = *entry;
        *entry = *parent;
        *parent = tempEntry;
        siftUp(parentIndex);
    }
}

template <class T>
bool
PriorityQueue<T>::add(const T& elem)
{
    return offer(elem);
}

template <class T>
bool
PriorityQueue<T>::offer(const T& elem)
{
    currentSize++;
    if (currentSize >= maxSize) {
        currentSize--;
        return false;
    }
    data[currentSize] = elem;
    siftUp(currentSize);
    return true;
}

template <class T>
T
PriorityQueue<T>::peek() const
{
    if (currentSize >= 1) {
        return data[1];
    }
    return T();
}

template <class T>
T
PriorityQueue<T>::poll()
{
    T top = peek();

    if (currentSize >= 1) {
        data[1] = data[currentSize--];
        if (currentSize >= 1) {
            siftDown(1);
        }
    }

    return top;
}

template <class T>
int
PriorityQueue<T>::size() const
{
    return currentSize;
}

template <class T>
void
PriorityQueue<T>::clear()
{
    reset();
}

template <class T>
inline PriorityQueue<T> *
PriorityQueue<T>::poolNext() const
{
    return nextQueue;
}

template <class T>
inline void
PriorityQueue<T>::setPoolNext(PriorityQueue<T> *next)
{
    nextQueue = next;
}

template <class T>
inline void
PriorityQueue<T>::resetForPool(int limit)
{
    maxSize = limit;
    reset();
}

#endif

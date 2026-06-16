#ifndef __PRIORITY_QUEUE_TXX__
#define __PRIORITY_QUEUE_TXX__

#include "java/util/PriorityQueue.h"

namespace java {

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
    if (initialCapacity <= 0) {
        initialCapacity = DEFAULT_CAPACITY;
    }

    currentSize = 0;
    data = nullptr;
    maxSize = 0;
    activeLimit = -1;
    allocateStorage(initialCapacity + 1);
}

template <class T>
void
PriorityQueue<T>::allocateStorage(int capacity)
{
    if (capacity < 2) {
        capacity = 2;
    }

    data = new T[capacity];
    maxSize = capacity;
}

template <class T>
void
PriorityQueue<T>::releaseStorage()
{
    if (data != nullptr) {
        delete[] data;
    }

    data = nullptr;
    currentSize = 0;
    maxSize = 0;
}

template <class T>
bool
PriorityQueue<T>::lessThan(const T& a, const T& b) const
{
    return a < b;
}

template <class T>
void
PriorityQueue<T>::ensureCapacity(int requiredCapacity)
{
    if (requiredCapacity < maxSize) {
        return;
    }

    int newCapacity = maxSize > 0 ? maxSize : DEFAULT_CAPACITY + 1;
    while (newCapacity <= requiredCapacity) {
        newCapacity = (newCapacity * 2) + 1;
    }

    T *oldData = data;
    allocateStorage(newCapacity);
    for (int i = 1; i <= currentSize; i++) {
        data[i] = oldData[i];
    }

    if (oldData != nullptr) {
        delete[] oldData;
    }
}

template <class T>
void
PriorityQueue<T>::siftUp(int index)
{
    if (index / 2 < 1) {
        return;
    }

    const int parentIndex = index / 2;
    if (lessThan(data[index], data[parentIndex])) {
        const T tempEntry = data[index];
        data[index] = data[parentIndex];
        data[parentIndex] = tempEntry;
        siftUp(parentIndex);
    }
}

template <class T>
void
PriorityQueue<T>::siftDown(int index)
{
    int childIndex = index * 2;
    if (childIndex <= currentSize) {
        if (childIndex + 1 <= currentSize &&
            lessThan(data[childIndex + 1], data[childIndex])) {
            childIndex++;
        }

        if (lessThan(data[childIndex], data[index])) {
            const T tempEntry = data[index];
            data[index] = data[childIndex];
            data[childIndex] = tempEntry;
            siftDown(childIndex);
        }
    }
}

template <class T>
bool
PriorityQueue<T>::add(const T& elem)
{
    if (activeLimit >= 0) {
        currentSize++;
        if (currentSize >= activeLimit) {
            currentSize--;
            return false;
        }
        data[currentSize] = elem;
        siftUp(currentSize);
        return true;
    }

    currentSize++;
    ensureCapacity(currentSize);
    data[currentSize] = elem;
    siftUp(currentSize);
    return true;
}

template <class T>
bool
PriorityQueue<T>::offer(const T& elem)
{
    return add(elem);
}

template <class T>
T
PriorityQueue<T>::peek()
{
    if (currentSize < 1) {
        return T();
    }
    return data[1];
}

template <class T>
T
PriorityQueue<T>::poll()
{
    if (currentSize < 1) {
        return T();
    }

    T top = data[1];
    data[1] = data[currentSize--];
    if (currentSize >= 1) {
        siftDown(1);
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
    currentSize = 0;
}

template <class T>
typename PriorityQueue<T>::Iterator
PriorityQueue<T>::iterator() const
{
    return Iterator(this, 1);
}

template <class T>
const T*
PriorityQueue<T>::begin() const
{
    return data + 1;
}

template <class T>
const T*
PriorityQueue<T>::end() const
{
    return data + currentSize + 1;
}

template <class T>
T*
PriorityQueue<T>::begin()
{
    return data + 1;
}

template <class T>
T*
PriorityQueue<T>::end()
{
    return data + currentSize + 1;
}

template <class T>
template <class Predicate>
bool
PriorityQueue<T>::removeIf(Predicate p)
{
    bool removed = false;
    int w = 1;
    for (int r = 1; r <= currentSize; r++) {
        if (p(data[r])) {
            removed = true;
        } else {
            data[w++] = data[r];
        }
    }
    if (removed) {
        currentSize = w - 1;
        for (int i = currentSize / 2; i >= 1; i--) {
            siftDown(i);
        }
    }
    return removed;
}

template <class T>
template <class Consumer>
void
PriorityQueue<T>::forEach(Consumer action) const
{
    for (int i = 1; i <= currentSize; i++) {
        action(data[i]);
    }
}

template <class T>
bool
PriorityQueue<T>::contains(const T& elem) const
{
    for (int i = 1; i <= currentSize; i++) {
        if (!(data[i] < elem) && !(elem < data[i])) {
            return true;
        }
    }
    return false;
}

template <class T>
bool
PriorityQueue<T>::remove(const T& elem)
{
    for (int i = 1; i <= currentSize; i++) {
        if (!(data[i] < elem) && !(elem < data[i])) {
            data[i] = data[currentSize--];
            if (currentSize >= 1 && i <= currentSize) {
                siftDown(i);
            }
            return true;
        }
    }
    return false;
}

template <class T>
T*
PriorityQueue<T>::toArray() const
{
    T* result = new T[currentSize];
    for (int i = 0; i < currentSize; i++) {
        result[i] = data[i + 1];
    }
    return result;
}

template <class T>
T*
PriorityQueue<T>::toArray(T* target) const
{
    for (int i = 0; i < currentSize; i++) {
        target[i] = data[i + 1];
    }
    return target;
}

}

#endif

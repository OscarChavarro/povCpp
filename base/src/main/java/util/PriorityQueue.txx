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
    allocateStorage(initialCapacity);
}

template <class T>
void
PriorityQueue<T>::allocateStorage(int capacity)
{
    if (capacity <= 0) {
        data = nullptr;
        maxSize = 0;
        return;
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
    if (requiredCapacity <= maxSize) {
        return;
    }

    int newCapacity = maxSize > 0 ? maxSize : DEFAULT_CAPACITY;
    while (newCapacity < requiredCapacity) {
        newCapacity = (newCapacity * 2) + 1;
    }

    T *oldData = data;
    allocateStorage(newCapacity);
    for (int i = 0; i < currentSize; i++) {
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
    T value = data[index];

    while (index > 0) {
        const int parent = (index - 1) / 2;
        if (!lessThan(value, data[parent])) {
            break;
        }

        data[index] = data[parent];
        index = parent;
    }

    data[index] = value;
}

template <class T>
void
PriorityQueue<T>::siftDown(int index)
{
    T value = data[index];

    while (true) {
        const int left = (index * 2) + 1;
        if (left >= currentSize) {
            break;
        }

        const int right = left + 1;
        int smallest = left;
        if (right < currentSize && lessThan(data[right], data[left])) {
            smallest = right;
        }
        if (!lessThan(data[smallest], value)) {
            break;
        }

        data[index] = data[smallest];
        index = smallest;
    }

    data[index] = value;
}

template <class T>
bool
PriorityQueue<T>::add(const T& elem)
{
    ensureCapacity(currentSize + 1);
    data[currentSize] = elem;
    siftUp(currentSize);
    currentSize++;
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
    if (currentSize <= 0) {
        return T();
    }
    return data[0];
}

template <class T>
T
PriorityQueue<T>::poll()
{
    if (currentSize <= 0) {
        return T();
    }

    T top = data[0];
    currentSize--;

    if (currentSize > 0) {
        data[0] = data[currentSize];
        siftDown(0);
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

}

#ifndef JAVA_UTIL_ARRAYDEQUE_H
#define JAVA_UTIL_ARRAYDEQUE_H

namespace java {

// Minimal FIFO double-ended queue, header-only (unlike ArrayList, which
// splits declaration/.txx) since it is only ever needed inside the
// concurrency package (Executors, ConcurrentLinkedQueue) for plain queue
// usage: addLast / peekFirst / removeFirst / isEmpty.
template <class T>
class ArrayDeque {
  private:
    struct Node {
        T value;
        Node *next;
        explicit Node(const T &v) : value(v), next(nullptr) {}
    };

    Node *head;
    Node *tail;
    long int count;

  public:
    ArrayDeque() : head(nullptr), tail(nullptr), count(0) {}

    ArrayDeque(const ArrayDeque &) = delete;
    ArrayDeque &operator=(const ArrayDeque &) = delete;

    ~ArrayDeque() { clear(); }

    void addLast(const T &item) {
        Node *node = new Node(item);
        if (tail == nullptr) {
            head = tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
        ++count;
    }

    T peekFirst() const { return head->value; }

    void removeFirst() {
        Node *old = head;
        head = head->next;
        if (head == nullptr) {
            tail = nullptr;
        }
        delete old;
        --count;
    }

    bool isEmpty() const { return head == nullptr; }

    long int size() const { return count; }

    void clear() {
        while (head != nullptr) {
            Node *old = head;
            head = head->next;
            delete old;
        }
        tail = nullptr;
        count = 0;
    }
};

} // namespace java

#endif

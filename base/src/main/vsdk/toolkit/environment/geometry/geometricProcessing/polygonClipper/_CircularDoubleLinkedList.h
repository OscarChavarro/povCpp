#ifndef __CIRCULAR_DOUBLE_LINKED_LIST__
#define __CIRCULAR_DOUBLE_LINKED_LIST__

#include "java/lang/String.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polygonClipper/_DoubleLinkedListNode.h"
template <class E>
class _CircularDoubleLinkedList {
private:
    _DoubleLinkedListNode<E>* head;
    int currentSize;

public:
    _CircularDoubleLinkedList() : head(0), currentSize(0) {}

    ~_CircularDoubleLinkedList() {
        while (currentSize > 0) remove(0);
    }

    int size() const { return currentSize; }

    void add(const E& data) {
        _DoubleLinkedListNode<E>* newNode = new _DoubleLinkedListNode<E>();
        newNode->data = data;
        if (head == 0) {
            newNode->previous = newNode;
            newNode->next = newNode;
            newNode->isHead = true;
            head = newNode;
            currentSize = 1;
        }
        else {
            newNode->previous = head->previous;
            newNode->next = head;
            head->previous->next = newNode;
            head->previous = newNode;
            ++currentSize;
        }
    }

    void add(int ind, const E& data) {
        if (head == 0 || ind >= currentSize) { add(data); return; }
        _DoubleLinkedListNode<E>* newNode = new _DoubleLinkedListNode<E>();
        newNode->data = data;
        if (ind == 0) {
            head->previous->next = newNode;
            newNode->previous = head->previous;
            newNode->next = head;
            newNode->isHead = true;
            head->isHead = false;
            head->previous = newNode;
            head = newNode;
            ++currentSize;
            return;
        }
        _DoubleLinkedListNode<E>* iterator = head;
        for (int i = 0; i < ind; ++i) iterator = iterator->next;
        iterator->previous->next = newNode;
        newNode->previous = iterator->previous;
        newNode->next = iterator;
        iterator->previous = newNode;
        ++currentSize;
    }

    void remove(int ind) {
        if (ind >= currentSize) {
            Logger::reportMessage("_CircularDoubleLinkedList", Logger::FATAL_ERROR,
                "remove", "Circ double linked list error: index out of bounds for remove operation.");
            return;
        }
        if (currentSize == 1) {
            delete head;
            head = 0;
            currentSize = 0;
            return;
        }
        _DoubleLinkedListNode<E>* target;
        if (ind == 0) {
            target = head;
            head->previous->next = head->next;
            head->next->previous = head->previous;
            head = head->next;
            head->isHead = true;
        }
        else {
            target = head;
            for (int i = 0; i < ind; ++i) target = target->next;
            target->previous->next = target->next;
            target->next->previous = target->previous;
        }
        delete target;
        --currentSize;
    }

    _DoubleLinkedListNode<E>* insertBefore(const E& data, _DoubleLinkedListNode<E>* node) {
        if (((node->isHead) && node != head) || head == 0) {
            Logger::reportMessage("_CircularDoubleLinkedList", Logger::FATAL_ERROR,
                "insertBefore", "Circ double linked list error: node not in this list.");
            return 0;
        }
        _DoubleLinkedListNode<E>* newNode = new _DoubleLinkedListNode<E>();
        newNode->data = data;
        if (node == head) {
            head->previous->next = newNode;
            newNode->isHead = true;
            newNode->previous = head->previous;
            newNode->next = head;
            head->isHead = false;
            head->previous = newNode;
            head = newNode;
        }
        else {
            node->previous->next = newNode;
            newNode->previous = node->previous;
            newNode->next = node;
            node->previous = newNode;
        }
        ++currentSize;
        return newNode;
    }

    _DoubleLinkedListNode<E>* getHead() const { return head; }
};

#endif

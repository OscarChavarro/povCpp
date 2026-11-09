#ifndef __DOUBLE_LINKED_LIST_NODE__
#define __DOUBLE_LINKED_LIST_NODE__

template <class E>
class _DoubleLinkedListNode {
public:
    E data;
    _DoubleLinkedListNode<E>* next;
    _DoubleLinkedListNode<E>* previous;
    bool isHead;

    _DoubleLinkedListNode() : next(0), previous(0), isHead(false) {}
};

#endif

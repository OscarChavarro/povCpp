#ifndef __ORG_W3C_DOM_NODE_LIST__
#define __ORG_W3C_DOM_NODE_LIST__

#include "java/util/ArrayList.h"
// Inline methods below instantiate the list
#include "java/util/ArrayList.txx"

namespace org {
namespace w3c {
namespace dom {

class Node;

/**
Minimal emulation of `org.w3c.dom.NodeList`: a non-owning view of nodes.
*/
class NodeList {
private:
    java::ArrayList<Node*> nodes;

public:
    NodeList() {}
    explicit NodeList(const java::ArrayList<Node*>& nodes) : nodes(nodes) {}

    int getLength() const
    {
        return (int)nodes.size();
    }

    /**
    @param index position of the node
    @return node at given position, or null if out of range
    */
    Node* item(int index) const
    {
        if ( index < 0 || index >= (int)nodes.size() ) {
            return nullptr;
        }
        return nodes.get(index);
    }
};

}
}
}

#endif

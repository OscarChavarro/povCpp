#ifndef __ORG_W3C_DOM_NODE__
#define __ORG_W3C_DOM_NODE__

#include "java/lang/String.h"
#include "java/util/ArrayList.h"

namespace org {
namespace w3c {
namespace dom {

class NodeList;
class NamedNodeMap;

/**
Minimal emulation of the `org.w3c.dom.Node` interface: element, attribute,
text and document nodes, as used by Vitral XML persistence classes. A node
owns its children and its attributes.
*/
class Node {
public:
    static const short ELEMENT_NODE = 1;
    static const short ATTRIBUTE_NODE = 2;
    static const short TEXT_NODE = 3;
    static const short DOCUMENT_NODE = 9;

    Node(short nodeType, const java::String& nodeName,
         const java::String& nodeValue);
    virtual ~Node();

    short getNodeType() const;
    const java::String& getNodeName() const;

    /**
    @return the local name for element and attribute nodes, null otherwise
    */
    const java::String* getLocalName() const;

    /**
    @return node value for attribute and text nodes, empty otherwise
    */
    const java::String& getNodeValue() const;
    void setNodeValue(const java::String& nodeValue);

    Node* getParentNode() const;
    Node* getFirstChild() const;
    NodeList getChildNodes() const;

    /**
    Appends a child, taking its ownership.
    @param child node to append; it must not have a parent
    @return the appended child
    */
    Node* appendChild(Node* child);

    /**
    @return attributes of element nodes, null for other node types
    */
    const NamedNodeMap* getAttributes() const;

    /**
    @return concatenation of the values of all descendant text nodes
    */
    java::String getTextContent() const;

    /**
    Replaces all children with a single text node.
    @param textContent new text
    */
    void setTextContent(const java::String& textContent);

protected:
    java::ArrayList<Node*> attributes;

private:
    short nodeType;
    java::String nodeName;
    java::String nodeValue;
    Node* parentNode;
    java::ArrayList<Node*> children;
    NamedNodeMap* attributeMap;

    Node(const Node& other);
    Node& operator=(const Node& other);

    friend class Element;
};

}
}
}

#endif

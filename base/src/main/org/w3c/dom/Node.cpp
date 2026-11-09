#include "java/util/ArrayList.txx"
#include "org/w3c/dom/Document.h"
#include "org/w3c/dom/Element.h"
#include "org/w3c/dom/NamedNodeMap.h"
#include "org/w3c/dom/Node.h"
#include "org/w3c/dom/NodeList.h"

namespace org {
namespace w3c {
namespace dom {

//= Node ======================================================================

Node::Node(short nodeType, const java::String& nodeName,
           const java::String& nodeValue)
    : nodeType(nodeType), nodeName(nodeName), nodeValue(nodeValue),
      parentNode(nullptr), attributeMap(nullptr)
{
    if ( nodeType == ELEMENT_NODE ) {
        attributeMap = new NamedNodeMap(&attributes);
    }
}

Node::~Node()
{
    long i;
    for ( i = 0; i < children.size(); i++ ) {
        delete children.get(i);
    }
    for ( i = 0; i < attributes.size(); i++ ) {
        delete attributes.get(i);
    }
    delete attributeMap;
}

short Node::getNodeType() const
{
    return nodeType;
}

const java::String& Node::getNodeName() const
{
    return nodeName;
}

const java::String* Node::getLocalName() const
{
    if ( nodeType != ELEMENT_NODE && nodeType != ATTRIBUTE_NODE ) {
        return nullptr;
    }
    return &nodeName;
}

const java::String& Node::getNodeValue() const
{
    return nodeValue;
}

void Node::setNodeValue(const java::String& value)
{
    nodeValue = value;
}

Node* Node::getParentNode() const
{
    return parentNode;
}

Node* Node::getFirstChild() const
{
    if ( children.size() == 0 ) {
        return nullptr;
    }
    return children.get(0);
}

NodeList Node::getChildNodes() const
{
    return NodeList(children);
}

Node* Node::appendChild(Node* child)
{
    if ( child == nullptr ) {
        return nullptr;
    }
    child->parentNode = this;
    children.add(child);
    return child;
}

const NamedNodeMap* Node::getAttributes() const
{
    return attributeMap;
}

java::String Node::getTextContent() const
{
    if ( nodeType == TEXT_NODE || nodeType == ATTRIBUTE_NODE ) {
        return nodeValue;
    }
    java::String text;
    long i;
    for ( i = 0; i < children.size(); i++ ) {
        text += children.get(i)->getTextContent();
    }
    return text;
}

void Node::setTextContent(const java::String& textContent)
{
    if ( nodeType == TEXT_NODE || nodeType == ATTRIBUTE_NODE ) {
        nodeValue = textContent;
        return;
    }
    long i;
    for ( i = 0; i < children.size(); i++ ) {
        delete children.get(i);
    }
    children.clear();
    appendChild(new Node(TEXT_NODE, "#text", textContent));
}

//= NamedNodeMap ==============================================================

int NamedNodeMap::getLength() const
{
    return (int)nodes->size();
}

Node* NamedNodeMap::item(int index) const
{
    if ( index < 0 || index >= (int)nodes->size() ) {
        return nullptr;
    }
    return nodes->get(index);
}

Node* NamedNodeMap::getNamedItem(const java::String& name) const
{
    long i;
    for ( i = 0; i < nodes->size(); i++ ) {
        if ( nodes->get(i)->getNodeName().equals(name) ) {
            return nodes->get(i);
        }
    }
    return nullptr;
}

//= Element ===================================================================

Element::Element(const java::String& tagName)
    : Node(ELEMENT_NODE, tagName, "")
{
}

const java::String& Element::getTagName() const
{
    return getNodeName();
}

java::String Element::getAttribute(const java::String& name) const
{
    Node* attribute = getAttributes()->getNamedItem(name);
    if ( attribute == nullptr ) {
        return java::String("");
    }
    return attribute->getNodeValue();
}

bool Element::hasAttribute(const java::String& name) const
{
    return getAttributes()->getNamedItem(name) != nullptr;
}

void Element::setAttribute(const java::String& name,
                           const java::String& value)
{
    Node* attribute = getAttributes()->getNamedItem(name);
    if ( attribute != nullptr ) {
        attribute->setNodeValue(value);
        return;
    }
    attribute = new Node(ATTRIBUTE_NODE, name, value);
    attribute->parentNode = this;
    attributes.add(attribute);
}

//= Document ==================================================================

Document::Document() : Node(DOCUMENT_NODE, "#document", "")
{
}

Element* Document::createElement(const java::String& tagName)
{
    return new Element(tagName);
}

Node* Document::createTextNode(const java::String& data)
{
    return new Node(TEXT_NODE, "#text", data);
}

Element* Document::getDocumentElement() const
{
    NodeList list = getChildNodes();
    int i;
    for ( i = 0; i < list.getLength(); i++ ) {
        if ( list.item(i)->getNodeType() == ELEMENT_NODE ) {
            return static_cast<Element*>(list.item(i));
        }
    }
    return nullptr;
}

static void collectElementsByTagName(const Node* node,
                                     const java::String& tagName,
                                     java::ArrayList<Node*>& out)
{
    NodeList list = node->getChildNodes();
    int i;
    for ( i = 0; i < list.getLength(); i++ ) {
        Node* child = list.item(i);
        if ( child->getNodeType() == Node::ELEMENT_NODE ) {
            if ( child->getNodeName().equals(tagName) ) {
                out.add(child);
            }
            collectElementsByTagName(child, tagName, out);
        }
    }
}

NodeList Document::getElementsByTagName(const java::String& tagName) const
{
    java::ArrayList<Node*> out;
    collectElementsByTagName(this, tagName, out);
    return NodeList(out);
}

void Document::normalizeDocument()
{
}

}
}
}

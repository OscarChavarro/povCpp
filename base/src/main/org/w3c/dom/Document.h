#ifndef __ORG_W3C_DOM_DOCUMENT__
#define __ORG_W3C_DOM_DOCUMENT__

#include "org/w3c/dom/Node.h"
#include "org/w3c/dom/NodeList.h"

namespace org {
namespace w3c {
namespace dom {

class Element;

/**
Minimal emulation of `org.w3c.dom.Document`. Nodes created by a document are
owned by the caller until appended to a node of the tree.
*/
class Document : public Node {
public:
    Document();
    virtual ~Document() {}

    Element* createElement(const java::String& tagName);
    Node* createTextNode(const java::String& data);

    /**
    @return the root element, or null for an empty document
    */
    Element* getDocumentElement() const;

    /**
    @param tagName name of the elements to find
    @return all descendant elements with given name, in document order
    */
    NodeList getElementsByTagName(const java::String& tagName) const;

    /**
    Merges adjacent text nodes (a no-op in this emulation, kept for API
    compatibility).
    */
    void normalizeDocument();
};

}
}
}

#endif

#ifndef __ORG_W3C_DOM_ELEMENT__
#define __ORG_W3C_DOM_ELEMENT__

#include "org/w3c/dom/Node.h"

namespace org {
namespace w3c {
namespace dom {

/**
Minimal emulation of `org.w3c.dom.Element`.
*/
class Element : public Node {
public:
    explicit Element(const java::String& tagName);
    virtual ~Element() {}

    const java::String& getTagName() const;

    /**
    @param name attribute name
    @return attribute value, or empty string if not present
    */
    java::String getAttribute(const java::String& name) const;
    bool hasAttribute(const java::String& name) const;
    void setAttribute(const java::String& name, const java::String& value);
};

}
}
}

#endif

#include <cstdio>

#include "java/util/ArrayList.txx"
#include "javax/xml/transform/Transformer.h"
#include "org/w3c/dom/Document.h"
#include "org/w3c/dom/Element.h"
#include "org/w3c/dom/NamedNodeMap.h"
#include "org/w3c/dom/NodeList.h"

namespace javax {
namespace xml {
namespace transform {

const char* const Transformer::DOCTYPE_SYSTEM = "doctype-system";
const char* const Transformer::INDENT = "indent";
const char* const Transformer::ENCODING = "encoding";

namespace {

java::String escape(const java::String& in, bool inAttribute)
{
    java::String out;
    char buffer[2] = {'\0', '\0'};
    int i;
    for ( i = 0; i < in.length(); i++ ) {
        char c = in.charAt(i);
        if ( c == '<' ) {
            out += "&lt;";
        }
        else if ( c == '>' ) {
            out += "&gt;";
        }
        else if ( c == '&' ) {
            out += "&amp;";
        }
        else if ( c == '"' && inAttribute ) {
            out += "&quot;";
        }
        else {
            buffer[0] = c;
            out += buffer;
        }
    }
    return out;
}

void writeIndent(java::String& out, bool indent, int level)
{
    if ( !indent ) {
        return;
    }
    int i;
    for ( i = 0; i < level; i++ ) {
        out += "    ";
    }
}

void writeNode(java::String& out, const org::w3c::dom::Node* node,
               bool indent, int level)
{
    if ( node->getNodeType() == org::w3c::dom::Node::TEXT_NODE ) {
        out += escape(node->getNodeValue(), false);
        return;
    }
    if ( node->getNodeType() != org::w3c::dom::Node::ELEMENT_NODE ) {
        return;
    }

    writeIndent(out, indent, level);
    out += "<";
    out += node->getNodeName();
    const org::w3c::dom::NamedNodeMap* attributes = node->getAttributes();
    int i;
    for ( i = 0; attributes != nullptr && i < attributes->getLength(); i++ ) {
        out += " ";
        out += attributes->item(i)->getNodeName();
        out += "=\"";
        out += escape(attributes->item(i)->getNodeValue(), true);
        out += "\"";
    }

    org::w3c::dom::NodeList children = node->getChildNodes();
    if ( children.getLength() == 0 ) {
        out += "/>";
        if ( indent ) {
            out += "\n";
        }
        return;
    }

    bool onlyText = true;
    for ( i = 0; i < children.getLength(); i++ ) {
        if ( children.item(i)->getNodeType() !=
             org::w3c::dom::Node::TEXT_NODE ) {
            onlyText = false;
        }
    }

    out += ">";
    if ( onlyText ) {
        for ( i = 0; i < children.getLength(); i++ ) {
            writeNode(out, children.item(i), indent, level + 1);
        }
    }
    else {
        if ( indent ) {
            out += "\n";
        }
        for ( i = 0; i < children.getLength(); i++ ) {
            writeNode(out, children.item(i), indent, level + 1);
        }
        writeIndent(out, indent, level);
    }
    out += "</";
    out += node->getNodeName();
    out += ">";
    if ( indent ) {
        out += "\n";
    }
}

}

Transformer::Transformer() : doctypeSystem(""), indent(false),
    encoding("UTF-8")
{
}

void Transformer::setOutputProperty(const java::String& name,
                                    const java::String& value)
{
    if ( name.equals(DOCTYPE_SYSTEM) ) {
        doctypeSystem = value;
    }
    else if ( name.equals(INDENT) ) {
        indent = value.equals("yes");
    }
    else if ( name.equals(ENCODING) ) {
        encoding = value;
    }
}

java::String Transformer::toXmlText(
    const org::w3c::dom::Document* document) const
{
    java::String out;
    out += "<?xml version=\"1.0\" encoding=\"";
    out += encoding;
    out += "\" standalone=\"no\"?>";
    const org::w3c::dom::Element* root = nullptr;
    if ( document != nullptr ) {
        root = document->getDocumentElement();
    }
    if ( root != nullptr && !doctypeSystem.isEmpty() ) {
        if ( indent ) {
            out += "\n";
        }
        out += "<!DOCTYPE ";
        out += root->getNodeName();
        out += " SYSTEM \"";
        out += doctypeSystem;
        out += "\">";
    }
    if ( indent ) {
        out += "\n";
    }
    if ( root != nullptr ) {
        writeNode(out, root, indent, 0);
    }
    return out;
}

bool Transformer::transform(const org::w3c::dom::Document* document,
                            const java::File& file) const
{
    java::String xmlText = toXmlText(document);
    FILE* fd = std::fopen(file.getPath().c_str(), "wb");
    if ( fd == nullptr ) {
        return false;
    }
    size_t n = (size_t)xmlText.length();
    bool ok = std::fwrite(xmlText.c_str(), 1, n, fd) == n;
    std::fclose(fd);
    return ok;
}

}
}
}

#ifndef __JAVAX_XML_PARSERS_DOCUMENT_BUILDER__
#define __JAVAX_XML_PARSERS_DOCUMENT_BUILDER__

#include "java/io/File.h"
#include "java/lang/String.h"

namespace org {
namespace w3c {
namespace dom {
class Document;
}
}
}

namespace javax {
namespace xml {
namespace parsers {

/**
Minimal emulation of `javax.xml.parsers.DocumentBuilder`: a small non
validating XML parser, enough for Vitral XML persistence files. It supports
elements, attributes, text, character/entity references, comments,
processing instructions, CDATA sections and a skipped DOCTYPE declaration.
Whitespace-only text nodes are not kept in the tree.
*/
class DocumentBuilder {
public:
    /**
    @return a new empty document, owned by the caller
    */
    static org::w3c::dom::Document* newDocument();

    /**
    @param file XML file to read
    @return a new document owned by the caller, or null if the file could
    not be read or is not well formed
    */
    static org::w3c::dom::Document* parse(const java::File& file);

    /**
    @param text XML text
    @return a new document owned by the caller, or null if the text is not
    well formed
    */
    static org::w3c::dom::Document* parseText(const java::String& text);
};

}
}
}

#endif

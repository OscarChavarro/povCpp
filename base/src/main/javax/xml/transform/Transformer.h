#ifndef __JAVAX_XML_TRANSFORM_TRANSFORMER__
#define __JAVAX_XML_TRANSFORM_TRANSFORMER__

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
namespace transform {

/**
Minimal emulation of an identity `javax.xml.transform.Transformer` that
serializes a DOM document into an XML file. Supported output properties are
the ones of `OutputKeys`: `doctype-system`, `indent` and `encoding`.
*/
class Transformer {
private:
    java::String doctypeSystem;
    bool indent;
    java::String encoding;

public:
    static const char* const DOCTYPE_SYSTEM;
    static const char* const INDENT;
    static const char* const ENCODING;

    Transformer();

    void setOutputProperty(const java::String& name,
                           const java::String& value);

    /**
    @param document document to serialize
    @return XML text of the document
    */
    java::String toXmlText(const org::w3c::dom::Document* document) const;

    /**
    @param document document to serialize
    @param file output file
    @return true on success
    */
    bool transform(const org::w3c::dom::Document* document,
                   const java::File& file) const;
};

}
}
}

#endif

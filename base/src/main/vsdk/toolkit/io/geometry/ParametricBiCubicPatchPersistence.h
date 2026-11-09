#ifndef __PARAMETRIC_BI_CUBIC_PATCH_PERSISTENCE__
#define __PARAMETRIC_BI_CUBIC_PATCH_PERSISTENCE__

namespace org {
namespace w3c {
namespace dom {
class Document;
class Element;
class Node;
}
}
}

class ParametricBiCubicPatch;

/**
XML persistence for `ParametricBiCubicPatch`.
*/
class ParametricBiCubicPatchPersistence {
public:
    static const char* const rootName;

    /**
    @param nodeRoot XML node with a `ParametricBiCubicPatch` element
    @return a new patch owned by the caller, or null if it has no type
    @throws XmlException if the node is not a patch
    */
    static ParametricBiCubicPatch* nodeToParametricBiCubicPatch(
        const org::w3c::dom::Node* nodeRoot);

    /**
    @param patch patch to export
    @param document document used to create the nodes
    @return a new element owned by the caller (until appended to the tree)
    */
    static org::w3c::dom::Element* toElement(ParametricBiCubicPatch* patch,
                                            org::w3c::dom::Document* document);
};

#endif

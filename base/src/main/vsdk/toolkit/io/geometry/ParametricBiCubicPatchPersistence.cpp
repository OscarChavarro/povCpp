#include <cstdlib>

#include "org/w3c/dom/Document.h"
#include "org/w3c/dom/Element.h"
#include "org/w3c/dom/NamedNodeMap.h"
#include "org/w3c/dom/NodeList.h"
#include "vsdk/toolkit/environment/geometry/curve/ParametricCurve.h"
#include "vsdk/toolkit/environment/geometry/surface/ParametricBiCubicPatch.h"
#include "vsdk/toolkit/io/XmlException.h"
#include "vsdk/toolkit/io/geometry/ParametricBiCubicPatchPersistence.h"
#include "vsdk/toolkit/io/geometry/ParametricCurvePersistence.h"

const char* const ParametricBiCubicPatchPersistence::rootName =
    "ParametricBiCubicPatch";

namespace {
const char* const patchAttributesNames[] = {
    "type", "approximationSteps"};
}

ParametricBiCubicPatch*
ParametricBiCubicPatchPersistence::nodeToParametricBiCubicPatch(
    const org::w3c::dom::Node* nodeRoot)
{
    if ( nodeRoot == nullptr || !nodeRoot->getNodeName().equals(rootName) ) {
        throw XmlException("The node no is a patch ");
    }
    ParametricBiCubicPatch* patch = nullptr;
    const org::w3c::dom::NamedNodeMap* atts = nodeRoot->getAttributes();
    if ( atts != nullptr ) {
        org::w3c::dom::Node* atributo;
        atributo = atts->getNamedItem(patchAttributesNames[0]);
        if ( atributo != nullptr ) {
            java::String type = atributo->getNodeValue();
            if ( !type.equals("") ) {
                patch = new ParametricBiCubicPatch();
                patch->setType(std::atoi(type.c_str()));
                atributo = atts->getNamedItem(patchAttributesNames[1]);
                if ( atributo != nullptr ) {
                    java::String approximationSteps = atributo->getNodeValue();
                    if ( !approximationSteps.equals("") ) {
                        patch->setApproximationSteps(
                            std::atoi(approximationSteps.c_str()));
                    }
                }

                org::w3c::dom::NodeList nodeList = nodeRoot->getChildNodes();
                // patch.contourCurve = the first curve node
                for ( int i = 0; i < nodeList.getLength(); i++ ) {
                    org::w3c::dom::Node* nodeCurve = nodeList.item(i);
                    patch->contourCurve = ParametricCurvePersistence::
                        nodeToParametricCurve(nodeCurve);
                    if ( patch->contourCurve != nullptr ) {
                        break;
                    }
                }
            }
        }
    }

    return patch;
}

org::w3c::dom::Element* ParametricBiCubicPatchPersistence::toElement(
    ParametricBiCubicPatch* patch, org::w3c::dom::Document* document)
{
    org::w3c::dom::Element* nodeRoot = document->createElement(rootName);

    nodeRoot->setAttribute(patchAttributesNames[1],
        java::String::valueOf(patch->getApproximationSteps()));
    nodeRoot->setAttribute(patchAttributesNames[0],
        java::String::valueOf(patch->getType()));

    if ( patch->contourCurve != nullptr ) {
        org::w3c::dom::Element* eCurve;
        eCurve = ParametricCurvePersistence::toElement(patch->contourCurve,
                                                       document);
        nodeRoot->appendChild(eCurve);
    }
    return nodeRoot;
}

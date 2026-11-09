#include <cstdio>
#include <cstdlib>

#include "java/util/ArrayList.txx"
#include "java/util/StringTokenizer.h"
#include "org/w3c/dom/Document.h"
#include "org/w3c/dom/Element.h"
#include "org/w3c/dom/NamedNodeMap.h"
#include "org/w3c/dom/NodeList.h"
#include "vsdk/toolkit/environment/geometry/curve/ParametricCurve.h"
#include "vsdk/toolkit/io/XmlException.h"
#include "vsdk/toolkit/io/geometry/ParametricCurvePersistence.h"

const char* const ParametricCurvePersistence::rootName = "ParametricCurve";

namespace {
const char* const nodesNames[] = {
    "Point",
    "Vector3Dd",
};
const char* const curveAttributesNames[] = {
    "approximationSteps"};
const char* const pointAttributesNames[] = {
    "type"};

// Full precision, so that exported coordinates read back exactly
java::String formatCoordinate(double value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.17g", value);
    return java::String(buffer);
}
}

ParametricCurve* ParametricCurvePersistence::nodeToParametricCurve(
    const org::w3c::dom::Node* nodeRoot)
{
    if ( nodeRoot == nullptr || !nodeRoot->getNodeName().equals(rootName) ) {
        throw XmlException("The node no is a curve ");
    }
    ParametricCurve* curve = new ParametricCurve();
    const org::w3c::dom::NamedNodeMap* atts = nodeRoot->getAttributes();
    if ( atts != nullptr ) {
        org::w3c::dom::Node* atributo;
        atributo = atts->getNamedItem(curveAttributesNames[0]);
        if ( atributo != nullptr ) {
            java::String approximationSteps = atributo->getNodeValue();
            if ( !approximationSteps.equals("") ) {
                curve->setApproximationSteps(
                    std::atoi(approximationSteps.c_str()));
            }
        }
    }

    org::w3c::dom::NodeList nodeList = nodeRoot->getChildNodes();
    for ( int i = 0; i < nodeList.getLength(); i++ ) {
        org::w3c::dom::Node* nodo;
        nodo = nodeList.item(i);

        org::w3c::dom::NodeList subNodos = nodo->getChildNodes();
        java::ArrayList<Vector3Dd> v3;
        for ( int j = 0; j < subNodos.getLength(); j++ ) {
            org::w3c::dom::Node* subNodo = subNodos.item(j);
            if ( subNodo->getLocalName() != nullptr &&
                 subNodo->getLocalName()->equals(nodesNames[1]) ) {
                java::StringTokenizer coors(subNodo->getTextContent());
                double coordinates[3] = {0.0, 0.0, 0.0};
                int k;
                for ( k = 0; k < 3 && coors.hasMoreTokens(); k++ ) {
                    coordinates[k] = std::atof(coors.nextToken().c_str());
                }
                v3.add(Vector3Dd(coordinates[0], coordinates[1],
                                 coordinates[2]));
            }
        }

        atts = nodo->getAttributes();
        if ( atts != nullptr ) {
            org::w3c::dom::Node* atributo =
                atts->getNamedItem(pointAttributesNames[0]);
            if ( atributo == nullptr ) {
                continue;
            }
            java::String type = atributo->getNodeValue();
            if ( !type.equals("") ) {
                curve->addPoint(v3, std::atoi(type.c_str()));
            }
        }
    }

    return curve;
}

org::w3c::dom::Element* ParametricCurvePersistence::toElement(
    ParametricCurve* curve, org::w3c::dom::Document* document)
{
    org::w3c::dom::Element* nodeCurve = document->createElement(rootName);

    nodeCurve->setAttribute(curveAttributesNames[0],
        java::String::valueOf(curve->getApproximationSteps()));
    for ( int i = 0; i < curve->getPointSize(); i++ ) {
        org::w3c::dom::Element* ePoint =
            document->createElement(nodesNames[0]);
        ePoint->setAttribute(pointAttributesNames[0],
            java::String::valueOf(curve->getPointType(i)));
        const java::ArrayList<Vector3Dd>& v3 = curve->getPointVector(i);
        for ( int j = 0; j < v3.size(); j++ ) {
            org::w3c::dom::Element* eVector =
                document->createElement(nodesNames[1]);
            const Vector3Dd& v = v3[j];
            eVector->setTextContent(formatCoordinate(v.x()) + " " +
                formatCoordinate(v.y()) + " " + formatCoordinate(v.z()));
            ePoint->appendChild(eVector);
        }
        nodeCurve->appendChild(ePoint);
    }

    return nodeCurve;
}

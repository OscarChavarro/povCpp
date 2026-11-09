#include "java/lang/String.h"

#include "java/util/ArrayList.txx"
#include "java/util/Collections.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_PolyhedralBoundedSolidTopologicalValidator.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

namespace {
const char* const CLASS_NAME = "_PolyhedralBoundedSolidTopologicalValidator";
}

bool _PolyhedralBoundedSolidTopologicalValidator::validateTopologicalIntegrity(
    PolyhedralBoundedSolid* solid)
{
    long int i;
    long int j;
    long int k;
    _PolyhedralBoundedSolidEdge* e;
    _PolyhedralBoundedSolidHalfEdge* h1;
    _PolyhedralBoundedSolidHalfEdge* h2;
    _PolyhedralBoundedSolidFace* f;
    _PolyhedralBoundedSolidLoop* l;

    for ( i = 0; i < solid->getEdgesList().size(); i++ ) {
        e = solid->getEdgesList().get(i);
        h1 = e->rightHalf;
        h2 = e->leftHalf;
        if ( h1 == 0 || h2 == 0 ) {
            Logger::reportMessage(CLASS_NAME, Logger::WARNING,
                "validateTopologicalIntegrity", "Edge with null halfedge!");
            return false;
        }
        if ( h1->parentLoop->parentFace->parentSolid !=
             h2->parentLoop->parentFace->parentSolid ) {
            Logger::reportMessage(CLASS_NAME, Logger::WARNING,
                "validateTopologicalIntegrity",
                "Edge belonging to two different solids!");
            return false;
        }
    }

    java::ArrayList<int> edgeCount = java::Collections::nCopies<int>(solid->getEdgesList().size(), 0);

    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        f = solid->getPolygonsList().get(i);
        for ( j = 0; j < f->boundariesList.size(); j++ ) {
            _PolyhedralBoundedSolidHalfEdge* he;
            _PolyhedralBoundedSolidHalfEdge* heStart;

            l = f->boundariesList.get(j);
            he = l->boundaryStartHalfEdge;
            if ( he == 0 ) {
                Logger::reportMessage(CLASS_NAME, Logger::WARNING,
                    "validateTopologicalIntegrity",
                    java::String("Loop without starting halfedge\n") +
                    "Offending solid:\n" + solid->toString());
                return false;
            }
            heStart = he;
            do {
                he = he->next();
                if ( he == 0 ) {
                    Logger::reportMessage(CLASS_NAME, Logger::WARNING,
                        "validateTopologicalIntegrity", "Not closed loop!");
                    return false;
                }
                for ( k = 0; k < (long int)edgeCount.size(); k++ ) {
                    if ( he->parentEdge == solid->getEdgesList().get(k) ) {
                        edgeCount[k]++;
                        break;
                    }
                }
            } while ( he != heStart );
        }
    }

    for ( i = 0; i < (long int)edgeCount.size(); i++ ) {
        if ( edgeCount[i] != 2 ) {
            Logger::reportMessage(CLASS_NAME, Logger::WARNING,
                "validateTopologicalIntegrity",
                "Edges with different halfedges than 2!");
            return false;
        }
    }
    return true;
}

void _PolyhedralBoundedSolidTopologicalValidator::remakeEmanatingHalfedgesReferences(
    PolyhedralBoundedSolid* solid)
{
    long int i;
    long int j;

    for ( i = 0; i < solid->getVerticesList().size(); i++ ) {
        solid->getVerticesList().get(i)->emanatingHalfEdge = 0;
    }

    for ( i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        for ( j = 0; j < face->boundariesList.size(); j++ ) {
            _PolyhedralBoundedSolidLoop* loop;
            _PolyhedralBoundedSolidHalfEdge* he;
            _PolyhedralBoundedSolidHalfEdge* heStart;

            loop = face->boundariesList.get(j);
            he = loop->boundaryStartHalfEdge;
            if ( he == 0 ) {
                continue;
            }
            heStart = he;
            do {
                he->startingVertex->emanatingHalfEdge = he;
                he = he->next();
                if ( he == 0 ) {
                    break;
                }
            } while ( he != heStart );
        }
    }

    // C++ port note: as the Java version, unreferenced vertices are only
    // unlisted from the solid; they are not deleted here
    for ( i = 0; i < solid->getVerticesList().size(); i++ ) {
        if ( solid->getVerticesList().get(i)->emanatingHalfEdge == 0 ) {
            solid->getVerticesList().remove(i);
            i--;
        }
    }
}

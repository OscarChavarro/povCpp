#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fallbacks/_PolyhedralBoundedSolidIdNamespace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

_PolyhedralBoundedSolidIdNamespace::_PolyhedralBoundedSolidIdNamespace(
    PolyhedralBoundedSolid* solidA, PolyhedralBoundedSolid* solidB)
{
    int maxV;
    int maxF;

    maxV = solidA->getMaxVertexId();
    if ( solidB->getMaxVertexId() > maxV ) {
        maxV = solidB->getMaxVertexId();
    }
    maxF = solidA->getMaxFaceId();
    if ( solidB->getMaxFaceId() > maxF ) {
        maxF = solidB->getMaxFaceId();
    }

    nextVertexIdValue = maxV + 1;
    nextFaceIdValue = maxF + 1;
}

int _PolyhedralBoundedSolidIdNamespace::nextVertexId(
    PolyhedralBoundedSolid* solidA, PolyhedralBoundedSolid* solidB)
{
    int id;

    id = nextVertexIdValue;
    nextVertexIdValue++;

    if ( id > solidA->getMaxVertexId() ) {
        solidA->setMaxVertexId(id);
    }
    if ( id > solidB->getMaxVertexId() ) {
        solidB->setMaxVertexId(id);
    }

    return id;
}

int _PolyhedralBoundedSolidIdNamespace::nextFaceId(
    PolyhedralBoundedSolid* solidA, PolyhedralBoundedSolid* solidB)
{
    int id;

    id = nextFaceIdValue;
    nextFaceIdValue++;

    if ( id > solidA->getMaxFaceId() ) {
        solidA->setMaxFaceId(id);
    }
    if ( id > solidB->getMaxFaceId() ) {
        solidB->setMaxFaceId(id);
    }

    return id;
}

int _PolyhedralBoundedSolidIdNamespace::nextFaceId(PolyhedralBoundedSolid* solid)
{
    int id;

    id = nextFaceIdValue;
    nextFaceIdValue++;

    if ( id > solid->getMaxFaceId() ) {
        solid->setMaxFaceId(id);
    }

    return id;
}

int _PolyhedralBoundedSolidIdNamespace::peekNextVertexId() const
{
    return nextVertexIdValue;
}

int _PolyhedralBoundedSolidIdNamespace::peekNextFaceId() const
{
    return nextFaceIdValue;
}

void _PolyhedralBoundedSolidIdNamespace::updmaxnames(
    PolyhedralBoundedSolid* solidToUpdate, PolyhedralBoundedSolid* referenceSolid)
{
    _PolyhedralBoundedSolidVertex* v;
    _PolyhedralBoundedSolidFace* f;
    long i;

    for ( i = 0; i < solidToUpdate->getVerticesList().size(); i++ ) {
        v = solidToUpdate->getVerticesList().get(i);
        v->id += referenceSolid->getMaxVertexId();
        if ( v->id > solidToUpdate->getMaxVertexId() ) {
            solidToUpdate->setMaxVertexId(v->id);
        }
    }

    for ( i = 0; i < solidToUpdate->getPolygonsList().size(); i++ ) {
        f = solidToUpdate->getPolygonsList().get(i);
        f->id += referenceSolid->getMaxFaceId();
        if ( f->id > solidToUpdate->getMaxFaceId() ) {
            solidToUpdate->setMaxFaceId(f->id);
        }
    }
}

int _PolyhedralBoundedSolidIdNamespace::nextVertexId(
    PolyhedralBoundedSolid* current, PolyhedralBoundedSolid* other,
    _PolyhedralBoundedSolidIdNamespace* ns)
{
    int a;
    int b;

    if ( ns != nullptr ) {
        return ns->nextVertexId(current, other);
    }
    a = current->getMaxVertexId();
    b = other->getMaxVertexId();
    return (b > a ? b : a) + 1;
}

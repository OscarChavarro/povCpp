#include "java/lang/String.h"
#include <cstdint>
#include <cstring>

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetOperatorVertexFace.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetOperatorVertexVertex.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/topology/_PolyhedralBoundedSolidSetOperatorNullEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

namespace {

/**
As Java `Double.compare`: a total order where -0.0 < 0.0 and NaN is the
greatest value (all NaNs equal).
*/
int javaDoubleCompare(double a, double b)
{
    if ( a < b ) {
        return -1;
    }
    if ( a > b ) {
        return 1;
    }
    int64_t aBits;
    int64_t bBits;
    if ( a != a ) {
        aBits = INT64_C(0x7ff8000000000000);
    }
    else {
        std::memcpy(&aBits, &a, sizeof(aBits));
    }
    if ( b != b ) {
        bBits = INT64_C(0x7ff8000000000000);
    }
    else {
        std::memcpy(&bBits, &b, sizeof(bBits));
    }
    return aBits == bBits ? 0 : (aBits < bBits ? -1 : 1);
}

/**
Exact lexicographic comparison of two 3-D points.  Using exact double
comparison (no epsilon band) ensures a total order so that sorting
sonea/soneb is deterministic.  After the post-Generate weld pass vertices
that are geometrically coincident share the exact same double values, so the
epsilon band is no longer needed for correctness and only introduced
non-determinism.
*/
int comparePoint(const Vector3Dd& a, const Vector3Dd& b)
{
    int cmpX = javaDoubleCompare(a.x(), b.x());
    if ( cmpX != 0 ) {
        return cmpX;
    }

    int cmpY = javaDoubleCompare(a.y(), b.y());
    if ( cmpY != 0 ) {
        return cmpY;
    }

    return javaDoubleCompare(a.z(), b.z());
}

Vector3Dd midpoint(_PolyhedralBoundedSolidEdge* edge)
{
    Vector3Dd r = edge->rightHalf->startingVertex->position;
    Vector3Dd l = edge->leftHalf->startingVertex->position;
    return Vector3Dd(
        (r.x() + l.x()) * 0.5,
        (r.y() + l.y()) * 0.5,
        (r.z() + l.z()) * 0.5);
}

Vector3Dd canonicalFirstEndpoint(_PolyhedralBoundedSolidEdge* edge)
{
    Vector3Dd right = edge->rightHalf->startingVertex->position;
    Vector3Dd left = edge->leftHalf->startingVertex->position;
    if ( comparePoint(right, left) <= 0 ) {
        return right;
    }
    return left;
}

Vector3Dd canonicalSecondEndpoint(_PolyhedralBoundedSolidEdge* edge)
{
    Vector3Dd right = edge->rightHalf->startingVertex->position;
    Vector3Dd left = edge->leftHalf->startingVertex->position;
    if ( comparePoint(right, left) <= 0 ) {
        return left;
    }
    return right;
}

java::String vectorText(const Vector3Dd& v)
{
    java::String* text = v.toString();
    java::String result(text->c_str());
    delete text;
    return result;
}

}

PolyhedralBoundedSolidNumericPolicy::ToleranceContext
    _PolyhedralBoundedSolidSetOperatorNullEdge::numericContext =
    PolyhedralBoundedSolidNumericPolicy::defaultContext();

_PolyhedralBoundedSolidSetOperatorNullEdge::_PolyhedralBoundedSolidSetOperatorNullEdge(
    _PolyhedralBoundedSolidEdge* e)
    : e(e)
{
}

void _PolyhedralBoundedSolidSetOperatorNullEdge::setNumericContext(
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext* context)
{
    if ( context == nullptr ) {
        numericContext = PolyhedralBoundedSolidNumericPolicy::defaultContext();
    }
    else {
        numericContext = *context;
    }
}

int _PolyhedralBoundedSolidSetOperatorNullEdge::compareTo(
    const _PolyhedralBoundedSolidSetOperatorNullEdge& other) const
{
    int cmp;

    cmp = comparePoint(canonicalFirstEndpoint(this->e),
                       canonicalFirstEndpoint(other.e));
    if ( cmp != 0 ) {
        return cmp;
    }
    cmp = comparePoint(canonicalSecondEndpoint(this->e),
                       canonicalSecondEndpoint(other.e));
    if ( cmp != 0 ) {
        return cmp;
    }
    return comparePoint(midpoint(this->e), midpoint(other.e));
}

bool _PolyhedralBoundedSolidSetOperatorNullEdge::operator<(
    const _PolyhedralBoundedSolidSetOperatorNullEdge& other) const
{
    return compareTo(other) < 0;
}

java::String _PolyhedralBoundedSolidSetOperatorNullEdge::toString() const
{
    java::String msg = java::String(e->toString().c_str()) +
        " (sorted with respect to segment " +
        vectorText(canonicalFirstEndpoint(this->e)) + " -> " +
        vectorText(canonicalSecondEndpoint(this->e)) + ")";
    return java::String(msg.c_str());
}

//= Records of the coincidences ===========================================

java::String _PolyhedralBoundedSolidSetOperatorVertexFace::toString() const
{
    java::String msg = "{" + java::String(v->toString().c_str()) + " / " +
        f->toString().c_str() + "}";
    return java::String(msg.c_str());
}

java::String _PolyhedralBoundedSolidSetOperatorVertexVertex::toString() const
{
    java::String msg = "(" + java::String(va->toString().c_str()) + ") / (" +
        vb->toString().c_str() + "}";
    return java::String(msg.c_str());
}

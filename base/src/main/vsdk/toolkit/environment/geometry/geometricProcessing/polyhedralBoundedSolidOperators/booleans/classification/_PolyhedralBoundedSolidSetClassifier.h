//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#ifndef ___POLYHEDRAL_BOUNDED_SOLID_SET_CLASSIFIER__
#define ___POLYHEDRAL_BOUNDED_SOLID_SET_CLASSIFIER__

#include "java/lang/String.h"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/_PolyhedralBoundedSolidOperator.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetVertexVertexClassifier.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/topology/_PolyhedralBoundedSolidSetOperatorNullEdge.h"

class PolyhedralBoundedSolid;
class _PolyhedralBoundedSolidFace;
class _PolyhedralBoundedSolidHalfEdge;
class _PolyhedralBoundedSolidVertex;
class _SetOperationContext;

/**
Classification stages, connection stages, and no-intersection containment
logic for the Boolean set-operations pipeline of chapter [MANT1988].15.

C++ counterpart of Java's `_PolyhedralBoundedSolidSetClassifier`. C++ port
note: work in progress, the no-intersection cases (delegated to
`_PolyhedralBoundedSolidSetNonIntersectingClassifier`) come with it.
*/
class _PolyhedralBoundedSolidSetClassifier : public _PolyhedralBoundedSolidOperator {
public:
    typedef _PolyhedralBoundedSolidSetVertexVertexClassifier::VertexVertexClassificationData
        VertexVertexClassificationData;

    static void runSetOpClassify(int op,
                                 PolyhedralBoundedSolid* inSolidA,
                                 PolyhedralBoundedSolid* inSolidB,
                                 int flags,
                                 _SetOperationContext& ctx);

    /**
    Constructs a vector along the bisector of the sector defined by `he`.
    that points inward the he's containing face. This adapts the sector
    bisector idea from problem [MANT1988].14.1 to the set-operations
    classifiers of chapter [MANT1988].15.
    */
    static Vector3Dd inside(_PolyhedralBoundedSolidHalfEdge* he);

private:
    static int debugFlags;

    static int compareToZero(double value);
    static java::String summarizeHalfEdge(_PolyhedralBoundedSolidHalfEdge* he);
    static java::String summarizeNullEdge(const _PolyhedralBoundedSolidSetOperatorNullEdge* edge);
    static java::String formatVertexVertexTraceContext(int op, int cursor,
        const java::String& caseName, const java::String& action, int type);
    static java::String labelSectorState(int state);
    static java::String summarizeSector(const VertexVertexClassificationData& data,
                                       int index);
    static void traceVertexVertexDecisionStep(
        const VertexVertexClassificationData& data, int op,
        const java::String& phase, int index,
        _PolyhedralBoundedSolidHalfEdge* ha1, _PolyhedralBoundedSolidHalfEdge* ha2,
        _PolyhedralBoundedSolidHalfEdge* hb1, _PolyhedralBoundedSolidHalfEdge* hb2);
    static void traceVertexVertexDecisionReason(int op, int cursor,
        const java::String& caseName, const java::String& reason,
        _PolyhedralBoundedSolidHalfEdge* ha1, _PolyhedralBoundedSolidHalfEdge* ha2,
        _PolyhedralBoundedSolidHalfEdge* hb1, _PolyhedralBoundedSolidHalfEdge* hb2);
    static _PolyhedralBoundedSolidHalfEdge* selectAlternativeBHalfEdge(
        const VertexVertexClassificationData& data, int sectb,
        _PolyhedralBoundedSolidHalfEdge* avoid);
    static _PolyhedralBoundedSolidHalfEdge* resolveBEndpointCandidate(
        const VertexVertexClassificationData& data, int sectorIndex,
        bool selectHb1, _PolyhedralBoundedSolidHalfEdge* hb1,
        _PolyhedralBoundedSolidHalfEdge* hb2);
    static void traceIntersectingSectorsSnapshot(
        const VertexVertexClassificationData& data, int cursor);
    static int pointInFace(_PolyhedralBoundedSolidFace* face, const Vector3Dd& point);
    static int resolveCoplanarVertexVertexClass(int op, bool sameOrientation,
                                                bool sideA);
    static int nextVertexId(PolyhedralBoundedSolid* current,
                            PolyhedralBoundedSolid* other);
    static bool sctrwitthin(const Vector3Dd& dir, const Vector3Dd& ref1,
                            const Vector3Dd& ref2, const Vector3Dd& ref12);
    static _PolyhedralBoundedSolidHalfEdge* recoverEdgeSequenceEndpointFromStrut(
        _PolyhedralBoundedSolidHalfEdge* endpoint, bool isFromEndpoint);
    static void separateEdgeSequence(_SetOperationContext& ctx,
        _PolyhedralBoundedSolidHalfEdge* from,
        _PolyhedralBoundedSolidHalfEdge* to,
        int type, const java::String& traceContext,
        PolyhedralBoundedSolid* inSolidA, PolyhedralBoundedSolid* inSolidB);
    static void flipNullEdgeOrientationForOpenSide(_SetOperationContext& ctx,
        _PolyhedralBoundedSolidHalfEdge* he, int type, bool orient,
        const java::String& traceContext,
        PolyhedralBoundedSolid* inSolidA, PolyhedralBoundedSolid* inSolidB);
    static bool nulledge(_PolyhedralBoundedSolidHalfEdge* he);
    static bool strutnulledge(_PolyhedralBoundedSolidHalfEdge* he);
    static bool convexedg(_PolyhedralBoundedSolidHalfEdge* he);
    static bool sectorwide(_PolyhedralBoundedSolidHalfEdge* he, int ind);
    static bool getOrientation(_PolyhedralBoundedSolidHalfEdge* ref,
                               _PolyhedralBoundedSolidHalfEdge* he1,
                               _PolyhedralBoundedSolidHalfEdge* he2);
    static bool sectorContainsAState(
        const _PolyhedralBoundedSolidSetOperatorSectorClassificationOnSector& sector,
        int wantedState);
    static bool sectorContainsBState(
        const _PolyhedralBoundedSolidSetOperatorSectorClassificationOnSector& sector,
        int wantedState);
    static _PolyhedralBoundedSolidHalfEdge* selectMissingEndpoint(
        const VertexVertexClassificationData& data, bool fromSolidA,
        int wantedState, _PolyhedralBoundedSolidHalfEdge* avoid);
    static void recoverMissingCoplanarEndpoints(
        const VertexVertexClassificationData& data,
        _PolyhedralBoundedSolidHalfEdge*& ha1, _PolyhedralBoundedSolidHalfEdge*& ha2,
        _PolyhedralBoundedSolidHalfEdge*& hb1, _PolyhedralBoundedSolidHalfEdge*& hb2);
    static void vertexVertexInsertNullEdges(_SetOperationContext& ctx,
        VertexVertexClassificationData& data, int op,
        PolyhedralBoundedSolid* inSolidA, PolyhedralBoundedSolid* inSolidB);
    static void vertexVertexClassify(_SetOperationContext& ctx,
        _PolyhedralBoundedSolidVertex* va, _PolyhedralBoundedSolidVertex* vb,
        int op, PolyhedralBoundedSolid* inSolidA, PolyhedralBoundedSolid* inSolidB);
    static void setOpClassify(_SetOperationContext& ctx, int op,
                              PolyhedralBoundedSolid* inSolidA,
                              PolyhedralBoundedSolid* inSolidB);
};

#endif

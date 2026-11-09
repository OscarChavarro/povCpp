//= References:                                                             =
//= [MANT1986] Mantyla Martti. "Boolean Operations of 2-Manifolds through   =
//=     Vertex Neighborhood Classification". ACM Transactions on Graphics,  =
//=     Vol. 5, No. 1, January 1986, pp. 1-29.                              =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =
//= [.wMANT2008] Mantyla Martti. "Personal Home Page", <<shar>> archive     =
//=     containing the C programs from [MANT1988]. Available at             =
//=     http://www.cs.hut.fi/~mam . Last visited April 12 / 2008.           =

#ifndef ___POLYHEDRAL_BOUNDED_SOLID_SET_OPERATOR__
#define ___POLYHEDRAL_BOUNDED_SOLID_SET_OPERATOR__

#include "java/lang/IllegalStateException.h"
#include "java/util/ArrayList.h"

#include "java/lang/String.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/_PolyhedralBoundedSolidOperator.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetNonIntersectingClassifier.h"

class PolyhedralBoundedSolid;
class _PolyhedralBoundedSolidHalfEdge;
class _SetOperationContext;
class _PolyhedralBoundedSolidIdNamespace;
class _PolyhedralBoundedSolidSetOperatorVertexFace;
class _PolyhedralBoundedSolidSetOperatorSectorClassificationOnVertex;

/**
This class encapsulates the set operations algorithms for boundary
representation solids in VitralSDK. Basically, this class implements the
original algorithm published in the paper [MANT1986] and in the second
part of the book [MANT1988].
The algorithm is structured in 5 big phases:
  0. Calculate vertex/face and vertex/vertex crossings.
  1. Classify and split for vertex/face cases.
  2. Classify and split for vertex/vertex cases.
  3. Connect.
  4. Finish.
Note that each big phase is controlled in a method (mark as "big phase" in
its documentation).

C++ port notes:
- `setOp` returns a new solid owned by the caller. As in Java the operands
  are consumed (they are modified and parts of them are moved into the
  result), but before returning, every node used by the result is unlisted
  from the operands, so the caller can delete the operands and the result
  independently.
- Java `IllegalStateException` is thrown as `java::IllegalStateException`.

C++ counterpart of Java's `_PolyhedralBoundedSolidSetOperator`.
*/
class _PolyhedralBoundedSolidSetOperator : public _PolyhedralBoundedSolidOperator {
public:
    /**
    Optional callback used to export internal state in graphical form.
    */
    typedef void (*DebugSolidExporter)(PolyhedralBoundedSolid* solid,
        const java::String& pattern);

    static void setDebugSolidExporter(DebugSolidExporter exporter);

    /**
    Procedure `updmaxnames` functionality is described on section
    [MANT1988].15.4. This method increments the face and vertex
    identifiers of `solidToUpdate` so that they do not overlap with
    `referenceSolid` identifiers.
    */
    static void updmaxnames(PolyhedralBoundedSolid* solidToUpdate,
        PolyhedralBoundedSolid* referenceSolid);

    /**
    Constructs a vector along the bisector of the sector defined by `he`.
    that points inward the he's containing face. This adapts the sector
    bisector idea from problem [MANT1988].14.1 to the set-operations
    classifiers of chapter [MANT1988].15.

    C++ port note: protected in Java, used by the classifiers of the same
    package; public here.
    */
    static Vector3Dd inside(_PolyhedralBoundedSolidHalfEdge* he);

    static bool colinearVectorsWithDirection(const Vector3Dd& a,
                                             const Vector3Dd& b);

    /**
    Outcome of the endpoint-pairing recovery loop in `separateEdgeSequence`.
    */
    enum SeparateEdgeSequenceResult {
        OK,
        FAILED_NULL_INPUT,
        FAILED_DIFFERENT_SOLIDS,
        FAILED_CYCLE_DETECTED,
        FAILED_NO_PAIRING_REACHED
    };

    /**
    Following program [MANT1988].15.12. Adapts the wMANT2008 recovery
    extensions for null-edge endpoints (cases A-E) using strict cycle
    detection. Retained (as in Java) only to validate the failure modes;
    the live pipeline uses the classifier version.
    @return diagnostic result; OK only when `from` and `to` share starting
    vertex and the LMEV split has been applied
    */
    static SeparateEdgeSequenceResult separateEdgeSequence(
        _PolyhedralBoundedSolidHalfEdge* from,
        _PolyhedralBoundedSolidHalfEdge* to,
        int type,
        PolyhedralBoundedSolid* inSolidA,
        PolyhedralBoundedSolid* inSolidB);

    /**
    @return true when `result` has faces, none degenerate, and passes the
    intermediate validation
    */
    static bool isStructurallyUsableSetOpResult(PolyhedralBoundedSolid* result);

    /**
    Following program [MANT1988].15.1.
    @param inSolidA first operand (consumed)
    @param inSolidB second operand (consumed)
    @param op UNION, INTERSECTION or SUBTRACT
    @param withDebug true to dump the pipeline state to files and stdout
    @param maximizeResultFaces true to maximize the faces of the result
    @param doStrictValidation true to enforce strict validation of the result
    @return the result solid, owned by the caller
    @throws java::IllegalStateException (Java IllegalStateException) when
    `doStrictValidation` is true and the result fails strict validation
    */
    static PolyhedralBoundedSolid* setOp(
        PolyhedralBoundedSolid* inSolidA,
        PolyhedralBoundedSolid* inSolidB,
        int op,
        bool withDebug = false,
        bool maximizeResultFaces = true,
        bool doStrictValidation = true);

private:
    typedef _PolyhedralBoundedSolidSetNonIntersectingClassifier::_PreflightCache PreflightCache;

    static const int DEBUG_01_STRUCTURE = 0x01;
    static const int DEBUG_02_GENERATOR = 0x02;
    static const int DEBUG_03_VERTEXFACECLASIFFIER = 0x04;
    static const int DEBUG_04_VERTEXVERTEXCLASIFFIER = 0x08;
    static const int DEBUG_05_CONNECT = 0x10;
    static const int DEBUG_06_FINISH = 0x20;
    static const int DEBUG_99_SHOWOPERATIONS = 0x40;
    static int debugFlags;
    static DebugSolidExporter debugSolidExporter;

    /** Namespace installed by the most recent setOp (owned here). */
    static _PolyhedralBoundedSolidIdNamespace* ownedNamespace;

    static int nextVertexId(PolyhedralBoundedSolid* current,
        PolyhedralBoundedSolid* other);
    static void setOpGenerate(_SetOperationContext& ctx,
        PolyhedralBoundedSolid* inSolidA, PolyhedralBoundedSolid* inSolidB);
    static void weldIntersectionVertices(_SetOperationContext& ctx,
        PolyhedralBoundedSolid* inSolidA, PolyhedralBoundedSolid* inSolidB);
    static void pruneStaleVertexFaceEntries(
        java::ArrayList<_PolyhedralBoundedSolidSetOperatorVertexFace>& list,
        PolyhedralBoundedSolid* solid);
    static bool sectoroverlap(
        const _PolyhedralBoundedSolidSetOperatorSectorClassificationOnVertex& na,
        const _PolyhedralBoundedSolidSetOperatorSectorClassificationOnVertex& nb);
    static _PolyhedralBoundedSolidHalfEdge* recoverEdgeSequenceEndpointFromStrut(
        _PolyhedralBoundedSolidHalfEdge* endpoint, bool isFromEndpoint);
    static bool nulledge(_PolyhedralBoundedSolidHalfEdge* he);
    static bool strutnulledge(_PolyhedralBoundedSolidHalfEdge* he);
    static void setOpClassify(_SetOperationContext& ctx, int op,
        PolyhedralBoundedSolid* inSolidA, PolyhedralBoundedSolid* inSolidB);
    static void traceSelfTouchingLoops(PolyhedralBoundedSolid* solid,
        const char* label);
    static void splitSelfTouchingLoops(PolyhedralBoundedSolid* solid);
    static void setOpConnect(_SetOperationContext& ctx, int op);
    static void setOpFinish(_SetOperationContext& ctx,
        PolyhedralBoundedSolid* inSolidA, PolyhedralBoundedSolid* inSolidB,
        PolyhedralBoundedSolid* outRes, int op);
    static void debugSolid(PolyhedralBoundedSolid* solid, const char* pattern);
    static void postProcessResult(PolyhedralBoundedSolid* res,
        bool maximizeResultFaces);
    static PolyhedralBoundedSolid* completeSetOpResult(
        PolyhedralBoundedSolid* result,
        const java::String& operandADiagnostics,
        const java::String& operandBDiagnostics,
        int op,
        const char* resultPath,
        bool maximizeResultFaces,
        bool doStrictValidation);
    static java::String operationName(int op);
    static java::String cardinalitiesAndBounds(PolyhedralBoundedSolid* solid);
    static PolyhedralBoundedSolid* deepCloneSolid(PolyhedralBoundedSolid* solid,
        const char* solidLabel);
    static bool hasDegenerateFace(PolyhedralBoundedSolid* solid);
    static bool shouldUseAxisAlignedCellBooleanFallback(
        PolyhedralBoundedSolid* fallback, PolyhedralBoundedSolid* result);
    static bool hasIncompleteConnectState();
    static bool hasBasicSetOpShapeData(PolyhedralBoundedSolid* result);
    static bool hasSameShapeData(PolyhedralBoundedSolid* first,
        PolyhedralBoundedSolid* second);

    /**
    C++ ownership helper: removes from the lists of `from` every face, edge
    and vertex that is also listed by `reference`.
    */
    static void unlistSharedNodes(PolyhedralBoundedSolid* from,
        PolyhedralBoundedSolid* reference);

    /**
    C++ ownership helper: deletes an intermediate result that is being
    replaced, after unlisting the nodes it shares with the operands.
    */
    static void discardIntermediateResult(PolyhedralBoundedSolid* result,
        PolyhedralBoundedSolid* inSolidA, PolyhedralBoundedSolid* inSolidB);
};

#endif

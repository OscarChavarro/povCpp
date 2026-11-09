//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =
//= [.wMANT2008] Mantyla Martti. "Personal Home Page", <<shar>> archive     =
//=     containing the C programs from [MANT1988]. Available at             =
//=     http://www.cs.hut.fi/~mam . Last visited April 12 / 2008.           =

#include "java/lang/String.h"

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/statistics/PolyhedralBoundedSolidStatistics.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

namespace {

const char* const CLASS_NAME = "PolyhedralBoundedSolidEulerOperators";
const char* const FACE_MESSAGE = "Face ";
const char* const EDGE_MESSAGE = "Edge ";
const char* const NOT_FOUND_MESSAGE = " not found.";
const char* const EDGE_NOT_FOUND_IN_FACE_WILDCARD_MESSAGE =
    " - * not found in face ";
const char* const EDGE_NOT_FOUND_IN_FACE_MESSAGE = " not found in face ";
const char* const LRINGMV_MESSAGE = "lringmv";
const char* const ADDHE_MESSAGE = "addhe";
const char* const DOT = ".";

void report(int level, const char* method, const java::String& message)
{
    Logger::reportMessage(CLASS_NAME, level, method, message);
}

java::String str(int value)
{
    return java::String(java::String::valueOf(value).c_str());
}

/**
Emulates `CircularDoubleLinkedList.locateWindowAtElem`.
@param list list to search
@param elem element to locate
@return element index, or -1 when it is not in the list
*/
template <typename T>
long indexOf(const java::ArrayList<T>& list, T elem)
{
    for ( long i = 0; i < list.size(); ++i ) {
        if ( list.get(i) == elem ) {
            return i;
        }
    }
    return -1;
}

/**
Emulates `CircularDoubleLinkedList.insertBefore`: when `pivot` is not in
the list or is its head, `elem` becomes the new head.
@param list target list
@param elem element to insert
@param pivot element before which `elem` is inserted
*/
template <typename T>
void insertBefore(java::ArrayList<T>& list, T elem, T pivot)
{
    long pos = indexOf(list, pivot);
    if ( pos < 0 ) {
        pos = 0;
    }
    list.add(pos, elem);
}

/**
Emulates `locateWindowAtElem` followed by `removeElemAtWindow`.
@param list target list
@param elem element to remove
@return true if the element was found and removed
*/
template <typename T>
bool removeElem(java::ArrayList<T>& list, T elem)
{
    long pos = indexOf(list, elem);
    if ( pos < 0 ) {
        return false;
    }
    list.remove(pos);
    return true;
}

/**
Emulates `CircularDoubleLinkedList.swapElements`.
@param list target list
@param a first element
@param b second element
*/
template <typename T>
void swapElems(java::ArrayList<T>& list, T a, T b)
{
    long ia = indexOf(list, a);
    long ib = indexOf(list, b);
    if ( ia < 0 || ib < 0 || ia == ib ) {
        return;
    }
    T tmp = list.get(ia);
    list.set(ia, list.get(ib));
    list.set(ib, tmp);
}

_PolyhedralBoundedSolidHalfEdge* addhe(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidEdge* e,
    _PolyhedralBoundedSolidVertex* v,
    _PolyhedralBoundedSolidHalfEdge* where,
    int sign)
{
    _PolyhedralBoundedSolidHalfEdge* he;

    (void)solid;
    if ( where == 0 ) {
        report(Logger::WARNING, ADDHE_MESSAGE,
            "Cannot create a half-edge because the reference half-edge is null.");
        return 0;
    }
    if ( where->parentLoop == 0 ) {
        report(Logger::WARNING, ADDHE_MESSAGE,
            "Cannot create a half-edge because the reference half-edge has no parent loop.");
        return 0;
    }
    if ( e == 0 ) {
        report(Logger::WARNING, ADDHE_MESSAGE,
            "Cannot create a half-edge because the target edge is null.");
        return 0;
    }

    if ( where->parentEdge == 0 ) {
        he = where;
    }
    else {
        he = new _PolyhedralBoundedSolidHalfEdge(v, where->parentLoop);
        insertBefore(where->parentLoop->halfEdgesList, he, where);
        he->startingVertex = v;
    }
    he->parentEdge = e;
    he->parentLoop = where->parentLoop;

    if ( sign == PolyhedralBoundedSolid::PLUS ) {
        e->leftHalf = he;
    }
    else {
        e->rightHalf = he;
    }

    return he;
}

void splitVertexNeighborhood(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidHalfEdge* he1,
    _PolyhedralBoundedSolidHalfEdge* he2,
    int vertexId,
    const Vector3Dd& p)
{
    _PolyhedralBoundedSolidHalfEdge* he;
    _PolyhedralBoundedSolidVertex* newVertex;
    _PolyhedralBoundedSolidEdge* newEdge;

    if ( vertexId > solid->getMaxVertexId() ) solid->setMaxVertexId(vertexId);

    newEdge = new _PolyhedralBoundedSolidEdge();
    solid->getEdgesList().add(newEdge);
    newVertex = new _PolyhedralBoundedSolidVertex(p, vertexId);
    solid->getVerticesList().add(newVertex);

    //-----------------------------------------------------------------
    he = he1;
    while ( he != he2 ) {
        he->startingVertex = newVertex;
        he = (he->mirrorHalfEdge())->next();
    }

    //-----------------------------------------------------------------
    addhe(solid, newEdge, newVertex, he2, PolyhedralBoundedSolid::PLUS);
    addhe(solid, newEdge, he2->startingVertex, he1, PolyhedralBoundedSolid::MINUS);

    //-----------------------------------------------------------------
    newVertex->emanatingHalfEdge = he2->previous();
    he2->startingVertex->emanatingHalfEdge = he2;
}

void insertLineDrawingEdge(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidHalfEdge* he,
    int vertexId,
    const Vector3Dd& p)
{
    _PolyhedralBoundedSolidVertex* oldVertex;
    _PolyhedralBoundedSolidVertex* newVertex;
    _PolyhedralBoundedSolidEdge* newEdge;

    if ( vertexId > solid->getMaxVertexId() ) solid->setMaxVertexId(vertexId);

    newEdge = new _PolyhedralBoundedSolidEdge();
    solid->getEdgesList().add(newEdge);
    newVertex = new _PolyhedralBoundedSolidVertex(p, vertexId);
    solid->getVerticesList().add(newVertex);
    oldVertex = he->startingVertex;

    addhe(solid, newEdge, oldVertex, he, PolyhedralBoundedSolid::PLUS);
    addhe(solid, newEdge, newVertex, he, PolyhedralBoundedSolid::MINUS);

    newVertex->emanatingHalfEdge = he->previous();
    oldVertex->emanatingHalfEdge = he;
}

bool failLringmv(const java::String& message)
{
    PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
    report(Logger::WARNING, LRINGMV_MESSAGE, message);
    return false;
}

_PolyhedralBoundedSolidFace* validateLringmvInput(
    _PolyhedralBoundedSolidLoop* loop,
    _PolyhedralBoundedSolidFace* destinationFace)
{
    if ( loop == 0 || destinationFace == 0 ) {
        failLringmv("Null input loop or destination face.");
        return 0;
    }

    _PolyhedralBoundedSolidFace* sourceFace = loop->parentFace;
    if ( sourceFace == 0 ) {
        failLringmv("Given loop does not have a parent face.");
        return 0;
    }
    if ( sourceFace->parentSolid != destinationFace->parentSolid ) {
        failLringmv("Loop move across different solids is not supported.");
        return 0;
    }
    return sourceFace;
}

void promoteLoopAsOuter(
    _PolyhedralBoundedSolidFace* face,
    _PolyhedralBoundedSolidLoop* loop)
{
    long numberOfLoops = face->boundariesList.size();
    if ( numberOfLoops <= 0 || face->boundariesList.get(0) == loop ) {
        return;
    }
    swapElems(face->boundariesList, loop, face->boundariesList.get(0));
}

bool demoteLoopAsInner(
    _PolyhedralBoundedSolidFace* face,
    _PolyhedralBoundedSolidLoop* loop)
{
    long numberOfLoops = face->boundariesList.size();
    if ( numberOfLoops <= 1 ) {
        return failLringmv("Cannot mark the only boundary loop as inner.");
    }
    if ( face->boundariesList.get(0) != loop ) {
        return true;
    }
    swapElems(face->boundariesList, loop, face->boundariesList.get(1));
    return true;
}

bool reorderLoopInSameFace(
    _PolyhedralBoundedSolidLoop* loop,
    _PolyhedralBoundedSolidFace* face,
    bool setAsOuterLoop)
{
    if ( indexOf(face->boundariesList, loop) < 0 ) {
        return failLringmv(
            "Given loop was not found in its parent face boundaries.");
    }
    if ( setAsOuterLoop ) {
        promoteLoopAsOuter(face, loop);
        return true;
    }
    return demoteLoopAsInner(face, loop);
}

bool moveLoopAcrossFaces(
    _PolyhedralBoundedSolidLoop* loop,
    _PolyhedralBoundedSolidFace* sourceFace,
    _PolyhedralBoundedSolidFace* destinationFace,
    bool setAsOuterLoop)
{
    if ( sourceFace->boundariesList.size() <= 1 ) {
        return failLringmv("Cannot move the only boundary loop of a face.");
    }
    if ( !setAsOuterLoop && destinationFace->boundariesList.size() <= 0 ) {
        return failLringmv(
            "Cannot insert an inner loop into a face without outer loop.");
    }
    if ( !removeElem(sourceFace->boundariesList, loop) ) {
        return failLringmv(
            "Given loop was not found in source face boundaries.");
    }

    loop->parentFace = destinationFace;

    if ( setAsOuterLoop ) {
        destinationFace->boundariesList.add(0L, loop);
    }
    else {
        destinationFace->boundariesList.add(loop);
    }
    return true;
}

} // namespace

//= LOW LEVEL EULER OPERATIONS ====================================

/**
mvfs: MakeVertexFaceSolid, as described in sections [MANT1988].9.2.2,
[MANT1988].11.3.1 and [MANT1988].11.5.1, following the structure of sample
program [MANT1988].11.5.
*/
void PolyhedralBoundedSolidEulerOperators::mvfs(PolyhedralBoundedSolid* solid, const Vector3Dd& p, int vertexId, int faceId)
{
    _PolyhedralBoundedSolidFace* newFace;
    _PolyhedralBoundedSolidLoop* newLoop;
    _PolyhedralBoundedSolidHalfEdge* newHalfEdge;
    _PolyhedralBoundedSolidVertex* newVertex;

    if ( vertexId > solid->getMaxVertexId() ) solid->setMaxVertexId(vertexId);
    if ( faceId > solid->getMaxFaceId() ) solid->setMaxFaceId(faceId);

    newFace = new _PolyhedralBoundedSolidFace(solid, faceId);
    solid->getPolygonsList().add(newFace);
    newLoop = new _PolyhedralBoundedSolidLoop(newFace);
    newVertex = new _PolyhedralBoundedSolidVertex(p, vertexId);
    solid->getVerticesList().add(newVertex);
    newHalfEdge = new _PolyhedralBoundedSolidHalfEdge(newVertex, newLoop);
    newLoop->halfEdgesList.add(newHalfEdge);
    newLoop->boundaryStartHalfEdge = newHalfEdge;
}

/**
kvfs: KillVertexFaceSolid, inverse of mvfs, as described in sections
[MANT1988].9.2.2 and [MANT1988].11.5.1. The solid must be the skeletal one.
*/
void PolyhedralBoundedSolidEulerOperators::kvfs(PolyhedralBoundedSolid* solid)
{
    if ( solid->getPolygonsList().size() != 1 ) {
        report(Logger::FATAL_ERROR, "kvfs",
            "Not skeletal solid, not having exactly one loop!");
        return;
    }
    if ( solid->getEdgesList().size() != 0 ) {
        report(Logger::FATAL_ERROR, "kvfs",
            "Not skeletal solid, having some edges!");
        return;
    }
    if ( solid->getVerticesList().size() != 1 ) {
        report(Logger::FATAL_ERROR, "kvfs",
            "Not skeletal solid, not having exactly one vertex !");
        return;
    }
    solid->getPolygonsList().remove(0L);
    solid->getVerticesList().remove(0L);
}

/**
lmev: low level make edge vertex (vertex splitting operation), as described
in sections [MANT1988].9.2.3, [MANT1988].11.3.2 and [MANT1988].11.5.1 and
following sample program [MANT1988].11.6. When `he1` and `he2` are the same
half-edge, the line-drawing strut is created.
*/
void PolyhedralBoundedSolidEulerOperators::lmev(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidHalfEdge* he1,
    _PolyhedralBoundedSolidHalfEdge* he2,
    int vertexId,
    const Vector3Dd& p)
{
    PolyhedralBoundedSolidStatistics::recordLmevCall();
    if ( he1 == 0 || he2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordInvalidHalfEdgeInputCase();
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lmev", "Calling with empty half-edge!");
        return;
    }
    if ( he1->startingVertex != he2->startingVertex ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::FATAL_ERROR, "lmev",
            "Half-edges not starting at the same vertex. Not supported case!");
        return;
    }
    if ( he1 == he2 ) {
        PolyhedralBoundedSolidStatistics::recordHe1EqualsHe2Case();
        insertLineDrawingEdge(solid, he1, vertexId, p);
        return;
    }
    splitVertexNeighborhood(solid, he1, he2, vertexId, p);
}

/**
lkev: low level kill edge vertex (vertex joining operation), inverse of
lmev, as described in sections [MANT1988].9.2.3, [MANT1988].11.3.4 and
[MANT1988].11.5.1. Answer to problem [MANT1988].11.3, borrowed from
[.wMANT2008].
*/
void PolyhedralBoundedSolidEulerOperators::lkev(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidHalfEdge* he1,
    _PolyhedralBoundedSolidHalfEdge* he2)
{
    PolyhedralBoundedSolidStatistics::recordLkevCall();
    //-----------------------------------------------------------------
    if ( he1 == 0 || he2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordInvalidHalfEdgeInputCase();
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lkev",
            "Two half-edges are needed for this Euler operator two work!");
        return;
    }
    if ( he1->parentEdge != he2->parentEdge ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lkev",
            "Given half-edges must lie over the same edge!");
        return;
    }
    if ( he1 == he2 ) {
        PolyhedralBoundedSolidStatistics::recordHe1EqualsHe2Case();
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lkev",
            "Given half-edges must be different!");
        return;
    }

    //-----------------------------------------------------------------
    // Answer borrowed from [.wMANT2008]
    _PolyhedralBoundedSolidHalfEdge* he;
    _PolyhedralBoundedSolidHalfEdge* he2next;

    he = he2->next();
    while ( he != he1 ) {
        he->startingVertex = he2->startingVertex;
        he = he->mirrorHalfEdge()->next();
    }

    he2next = he2->next();
    he1->parentLoop->unlistHalfEdge(he1);
    he2->parentLoop->unlistHalfEdge(he2);
    he2->startingVertex->emanatingHalfEdge = he2next;
    if ( he2->parentLoop->halfEdgesList.size() < 1 ) {
        he2->startingVertex->emanatingHalfEdge = 0;
    }

    removeElem(solid->getEdgesList(), he1->parentEdge);
    removeElem(solid->getVerticesList(), he1->startingVertex);

    if ( he2->parentLoop->halfEdgesList.size() <= 0 ) {
        he2->parentEdge = 0;
        he2->parentLoop->halfEdgesList.add(he2);
        he2->parentLoop->boundaryStartHalfEdge = he2;
    }
}

/**
lkef: Low Level Kill Edge Face, inverse of lmef. Removes the edge of `he1`
and `he2` and joins their two faces; `he2.parentLoop.parentFace` is removed.
*/
void PolyhedralBoundedSolidEulerOperators::lkef(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidHalfEdge* he1,
    _PolyhedralBoundedSolidHalfEdge* he2)
{
    PolyhedralBoundedSolidStatistics::recordLkefCall();
    if ( he1->parentEdge != he2->parentEdge ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::FATAL_ERROR, "lkef",
            "Given half-edges must lie over the same edge. Operation aborted.");
        return;
    }
    if ( he1->parentLoop->parentFace == he2->parentLoop->parentFace ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::FATAL_ERROR, "lkef",
            "Given half-edges must belong to different faces. Operation aborted.");
        return;
    }

    _PolyhedralBoundedSolidEdge* edgeToBeKilled;
    _PolyhedralBoundedSolidLoop* loopToBeKilled;
    _PolyhedralBoundedSolidFace* faceToBeKilled;
    _PolyhedralBoundedSolidHalfEdge* halfEdgePivot;

    halfEdgePivot = he1->next();
    edgeToBeKilled = he1->parentEdge;
    loopToBeKilled = he2->parentLoop;
    faceToBeKilled = loopToBeKilled->parentFace;

    java::ArrayList<_PolyhedralBoundedSolidHalfEdge*> migratedHalfEdges;

    _PolyhedralBoundedSolidHalfEdge* he;
    he = he2->next();
    long maxTraversal = he2->parentLoop->halfEdgesList.size() + 1;
    long traversed = 0;
    while ( he != he2 ) {
        if ( he == 0 || traversed > maxTraversal ) {
            PolyhedralBoundedSolidStatistics::recordConsistencyWarningCase();
            report(Logger::WARNING, "lkef",
                "Detected inconsistent next-chain; falling back to halfEdgesList order.");
            migratedHalfEdges.clear();
            long idx;
            for ( idx = 0; idx < he2->parentLoop->halfEdgesList.size(); idx++ ) {
                _PolyhedralBoundedSolidHalfEdge* candidate;
                candidate = he2->parentLoop->halfEdgesList.get(idx);
                if ( candidate != he2 ) {
                    migratedHalfEdges.add(candidate);
                }
            }
            break;
        }
        migratedHalfEdges.add(he);
        he = he->next();
        traversed++;
    }

    he1->parentLoop->unlistHalfEdge(he1);
    he2->parentLoop->unlistHalfEdge(he2);
    removeElem(solid->getEdgesList(), edgeToBeKilled);

    removeElem(faceToBeKilled->boundariesList, loopToBeKilled);
    removeElem(solid->getPolygonsList(), faceToBeKilled);

    long i;
    for ( i = migratedHalfEdges.size() - 1; i >= 0; i-- ) {
        he = migratedHalfEdges.get(i);
        he->parentLoop = he1->parentLoop;
        insertBefore(he1->parentLoop->halfEdgesList, he, halfEdgePivot);
        halfEdgePivot = he;
    }

    // If the killed face had additional loops (inner rings), transfer them
    // to the surviving face so their half-edges remain reachable.
    while ( faceToBeKilled->boundariesList.size() > 0 ) {
        _PolyhedralBoundedSolidLoop* orphanedLoop;
        orphanedLoop = faceToBeKilled->boundariesList.get(0);
        orphanedLoop->parentFace = he1->parentLoop->parentFace;
        faceToBeKilled->boundariesList.remove(0L);
        he1->parentLoop->parentFace->boundariesList.add(orphanedLoop);
    }
}

/**
lmef: low level make edge face (face splitting operator), as described in
sections [MANT1988].9.2.3, [MANT1988].11.3.3 and [MANT1988].11.5.1 and
following sample program [MANT1988].11.7. `he1` ends in the new face.
*/
_PolyhedralBoundedSolidFace* PolyhedralBoundedSolidEulerOperators::lmef(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidHalfEdge* he1,
    _PolyhedralBoundedSolidHalfEdge* he2,
    int newFaceId)
{
    PolyhedralBoundedSolidStatistics::recordLmefCall();
    _PolyhedralBoundedSolidFace* newFace;
    _PolyhedralBoundedSolidLoop* newLoop;
    _PolyhedralBoundedSolidLoop* oldLoop;
    _PolyhedralBoundedSolidEdge* newEdge;
    _PolyhedralBoundedSolidHalfEdge* he;
    _PolyhedralBoundedSolidHalfEdge* nhe1;
    _PolyhedralBoundedSolidHalfEdge* nhe2;

    if ( he1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordInvalidHalfEdgeInputCase();
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lmef",
            "Cannot split face because first half-edge is null.");
        return 0;
    }
    if ( he2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordInvalidHalfEdgeInputCase();
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lmef",
            "Cannot split face because second half-edge is null.");
        return 0;
    }
    if ( he1->parentLoop == 0 || he2->parentLoop == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lmef",
            "Cannot split face because one input half-edge has no parent loop.");
        return 0;
    }
    if ( he1->startingVertex == 0 || he2->startingVertex == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lmef",
            "Cannot split face because one input half-edge has null starting vertex.");
        return 0;
    }

    if ( newFaceId > solid->getMaxFaceId() ) solid->setMaxFaceId(newFaceId);

    newFace = new _PolyhedralBoundedSolidFace(solid, newFaceId);
    solid->getPolygonsList().add(newFace);
    oldLoop = he1->parentLoop;
    newLoop = new _PolyhedralBoundedSolidLoop(newFace);
    newEdge = new _PolyhedralBoundedSolidEdge();
    solid->getEdgesList().add(newEdge);

    java::ArrayList<_PolyhedralBoundedSolidHalfEdge*> migratedHalfEdges;

    he = he1;
    while ( he != he2 ) {
        migratedHalfEdges.add(he);
        he = he->next();
        if ( he == he1 ) break;
    }

    long i;
    for ( i = 0; i < migratedHalfEdges.size(); i++ ) {
        he = migratedHalfEdges.get(i);
        he->parentLoop = newLoop;
        oldLoop->unlistHalfEdge(he);
        newLoop->halfEdgesList.add(he);
    }

    nhe1 = addhe(solid, newEdge, he2->startingVertex, he1, PolyhedralBoundedSolid::MINUS);
    nhe2 = addhe(solid, newEdge, he1->startingVertex, he2, PolyhedralBoundedSolid::PLUS);
    if ( nhe1 == 0 || nhe2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lmef",
            "Cannot split face because one generated half-edge is null.");
        return 0;
    }

    newLoop->boundaryStartHalfEdge = nhe1;
    he2->parentLoop->boundaryStartHalfEdge = nhe2;

    return newFace;
}

/**
lkemr: low level kill edge make ring (loop splitting operator), as
described in sections [MANT1988].9.2.3 and [MANT1988].11.3.4, following
sample program [MANT1988].11.8.
*/
void PolyhedralBoundedSolidEulerOperators::lkemr(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidHalfEdge* he1,
    _PolyhedralBoundedSolidHalfEdge* he2)
{
    PolyhedralBoundedSolidStatistics::recordLkemrCall();
    //-----------------------------------------------------------------
    _PolyhedralBoundedSolidHalfEdge* he3;
    _PolyhedralBoundedSolidHalfEdge* he4;
    _PolyhedralBoundedSolidLoop* newLoop;
    _PolyhedralBoundedSolidLoop* oldLoop;
    _PolyhedralBoundedSolidEdge* killedEdge;

    oldLoop = he1->parentLoop;
    newLoop = new _PolyhedralBoundedSolidLoop(oldLoop->parentFace);
    killedEdge = he1->parentEdge;

    //-----------------------------------------------------------------
    java::ArrayList<_PolyhedralBoundedSolidHalfEdge*> migratedHalfEdges;

    he4 = he1->next();
    do {
        migratedHalfEdges.add(he4);
        if ( he4 == he2 ) break;
    } while ( (he4 = he4->next()) != he2 );

    //-----------------------------------------------------------------
    long i;

    for ( i = 0; i < migratedHalfEdges.size(); i++ ) {
        he3 = migratedHalfEdges.get(i);
        removeElem(oldLoop->halfEdgesList, he3);
        newLoop->halfEdgesList.add(he3);
        he3->parentLoop = newLoop;
    }
    newLoop->boundaryStartHalfEdge = newLoop->halfEdgesList.get(0);

    //-----------------------------------------------------------------
    oldLoop->delhe(he1);
    oldLoop->delhe(he2);

    if ( newLoop->halfEdgesList.size() <= 1 ) {
        newLoop->boundaryStartHalfEdge->parentEdge = 0;
        if ( newLoop->halfEdgesList.size() <= 0 ) {
            report(Logger::FATAL_ERROR, "lkemr", "Case A Should not happen!");
        }
        newLoop->halfEdgesList.get(0)->startingVertex->emanatingHalfEdge = 0;
    }

    if ( oldLoop->halfEdgesList.size() <= 1 ) {
        oldLoop->boundaryStartHalfEdge->parentEdge = 0;
        if ( oldLoop->halfEdgesList.size() <= 0 ) {
            report(Logger::FATAL_ERROR, "lkemr", "Case B Should not happen!");
        }
        oldLoop->halfEdgesList.get(0)->startingVertex->emanatingHalfEdge = 0;
    }

    removeElem(solid->getEdgesList(), killedEdge);
}

/**
lkfmrh: low level kill face make ring hole in same shell, from exercise
[MANT1988].11.5 and section [MANT1988].11.5.1. `face2` must be simple.
*/
void PolyhedralBoundedSolidEulerOperators::lkfmrh(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidFace* face1,
    _PolyhedralBoundedSolidFace* face2)
{
    if ( face2->boundariesList.size() > 1 ) {
        report(Logger::WARNING, "lkfmrh",
            "Internal face to form new loop must have just one boundary!");
        return;
    }

    _PolyhedralBoundedSolidLoop* newLoop;
    _PolyhedralBoundedSolidLoop* oldLoop;
    _PolyhedralBoundedSolidHalfEdge* he;
    long i;

    oldLoop = face2->boundariesList.get(0);
    newLoop = new _PolyhedralBoundedSolidLoop(face1);

    for ( i = 0; i < oldLoop->halfEdgesList.size(); i++ ) {
        he = oldLoop->halfEdgesList.get(i);
        he->parentLoop = newLoop;
        newLoop->halfEdgesList.add(he);
    }
    newLoop->boundaryStartHalfEdge = newLoop->halfEdgesList.get(0);

    removeElem(solid->getPolygonsList(), face2);
}

/**
lmfkrh: low level make face kill ring hole, inverse of lkfmrh, from
exercise [MANT1988].11.5 and section [MANT1988].11.5.1.
*/
_PolyhedralBoundedSolidFace* PolyhedralBoundedSolidEulerOperators::lmfkrh(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidLoop* l,
    int newFaceId)
{
    _PolyhedralBoundedSolidFace* newFace;
    newFace = new _PolyhedralBoundedSolidFace(solid, newFaceId);
    solid->getPolygonsList().add(newFace);

    if ( newFaceId > solid->getMaxFaceId() ) solid->setMaxFaceId(newFaceId);

    removeElem(l->parentFace->boundariesList, l);
    l->parentFace = newFace;
    newFace->boundariesList.add(l);

    return newFace;
}

/**
lkimrh: naming alias of lkfmrh for the [MANT1988].11.5 operator family.
*/
void PolyhedralBoundedSolidEulerOperators::lkimrh(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidFace* face1,
    _PolyhedralBoundedSolidFace* face2)
{
    lkfmrh(solid, face1, face2);
}

/**
lmikrh: naming alias of lmfkrh for the [MANT1988].11.5 operator family.
*/
_PolyhedralBoundedSolidFace* PolyhedralBoundedSolidEulerOperators::lmikrh(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidLoop* l,
    int newFaceId)
{
    return lmfkrh(solid, l, newFaceId);
}

/**
lringmv moves the loop `l` from its parent face to `toFace`, as inner loop
or as outer loop following `setAsOuterLoop`. It is an addendum to lmef, not
an Euler operator, following section [MANT1988].11.5.1.
*/
bool PolyhedralBoundedSolidEulerOperators::lringmv(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidLoop* l,
    _PolyhedralBoundedSolidFace* toFace,
    bool setAsOuterLoop)
{
    PolyhedralBoundedSolidStatistics::recordLringmvCall();
    (void)solid;
    _PolyhedralBoundedSolidFace* fromFace = validateLringmvInput(l, toFace);
    if ( fromFace == 0 ) {
        return false;
    }
    if ( fromFace == toFace ) {
        return reorderLoopInSameFace(l, toFace, setAsOuterLoop);
    }
    return moveLoopAcrossFaces(l, fromFace, toFace, setAsOuterLoop);
}

/**
lmekr: low level make edge kill ring, inverse of lkemr. Answer to exercise
[MANT1988].11.4, with the signature from section [MANT1988].11.5.1.
*/
void PolyhedralBoundedSolidEulerOperators::lmekr(
    PolyhedralBoundedSolid* solid,
    _PolyhedralBoundedSolidHalfEdge* he1,
    _PolyhedralBoundedSolidHalfEdge* he2)
{
    PolyhedralBoundedSolidStatistics::recordLmekrCall();
    //-----------------------------------------------------------------
    if ( he1->parentLoop == he2->parentLoop ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lmekr",
            "Given half-edges are on the same loop. Operation aborted.");
        return;
    }
    if ( he1->parentLoop->parentFace != he2->parentLoop->parentFace ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "lmekr",
            "Given half-edges are not on the same face. Operation aborted.");
        return;
    }

    //-----------------------------------------------------------------
    java::ArrayList<_PolyhedralBoundedSolidHalfEdge*> migratedHalfEdges;
    _PolyhedralBoundedSolidHalfEdge* he;
    _PolyhedralBoundedSolidLoop* ringToKill;

    ringToKill = he2->parentLoop;

    he = he2;
    do {
        migratedHalfEdges.add(he);
        he = he->next();
    } while ( he != he2 );

    //-----------------------------------------------------------------
    long i;

    // The ring is discarded: its half-edges migrate to `he1` loop below
    ringToKill->halfEdgesList.clear();
    removeElem(ringToKill->parentFace->boundariesList, ringToKill);

    //-----------------------------------------------------------------
    _PolyhedralBoundedSolidEdge* newEdge;
    _PolyhedralBoundedSolidVertex* v1;
    _PolyhedralBoundedSolidVertex* v2;

    v1 = he1->startingVertex;
    v2 = he2->startingVertex;

    newEdge = new _PolyhedralBoundedSolidEdge();
    solid->getEdgesList().add(newEdge);

    _PolyhedralBoundedSolidHalfEdge* heLast;

    heLast = addhe(solid, newEdge, v2, he1, PolyhedralBoundedSolid::MINUS);
    newEdge->rightHalf = addhe(solid, newEdge, v1, heLast, PolyhedralBoundedSolid::MINUS);
    newEdge->leftHalf = heLast;

    // Alas! This rare condition of not adding a migrated half edges
    // list of size 1 is to avoid adding an 0-length half-edge with no
    // parent edge and no mirror edge when loop is of size 1 vertex.
    for ( i = 0;
          i < migratedHalfEdges.size() && migratedHalfEdges.size() > 1;
          i++ ) {
        he = migratedHalfEdges.get(i);
        he->parentLoop = he1->parentLoop;
        insertBefore(he1->parentLoop->halfEdgesList, he, heLast);
    }
}

//= HIGH LEVEL EULER OPERATIONS ===================================

/**
smev: "Strut" or line-drawing "Simplified" version of mev operator.
*/
bool PolyhedralBoundedSolidEulerOperators::smev(
    PolyhedralBoundedSolid* solid, int f1, int v1, int v4, const Vector3Dd& p)
{
    _PolyhedralBoundedSolidFace* oldFace1;
    _PolyhedralBoundedSolidHalfEdge* he1;

    oldFace1 = solid->findFace(f1);
    if ( oldFace1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "mev", FACE_MESSAGE + str(f1) + NOT_FOUND_MESSAGE);
        return false;
    }
    he1 = oldFace1->findHalfEdge(v1);
    if ( he1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "mev",
            EDGE_MESSAGE + str(v1) + EDGE_NOT_FOUND_IN_FACE_WILDCARD_MESSAGE +
            str(f1) + DOT);
        return false;
    }
    lmev(solid, he1, he1, v4, p);
    return true;
}

/**
mev: (high level version) make edge vertex (vertex splitting operation).
Edges from `v1` -> `v2` (inclusive) to `v1` -> `v3` (exclusive) become
adjacent to the new vertex `v4`.
*/
bool PolyhedralBoundedSolidEulerOperators::mev(
    PolyhedralBoundedSolid* solid, int f1, int f2,
    int v1, int v2, int v3, int v4, const Vector3Dd& p)
{
    _PolyhedralBoundedSolidFace* oldFace1;
    _PolyhedralBoundedSolidFace* oldFace2;
    _PolyhedralBoundedSolidHalfEdge* he1;
    _PolyhedralBoundedSolidHalfEdge* he2;

    oldFace1 = solid->findFace(f1);
    if ( oldFace1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "mev", FACE_MESSAGE + str(f1) + NOT_FOUND_MESSAGE);
        return false;
    }
    oldFace2 = solid->findFace(f2);
    if ( oldFace2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "mev", FACE_MESSAGE + str(f2) + NOT_FOUND_MESSAGE);
        return false;
    }
    he1 = oldFace1->findHalfEdge(v1, v2);
    if ( he1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "mev",
            EDGE_MESSAGE + str(v1) + " - " + str(v2) +
            EDGE_NOT_FOUND_IN_FACE_MESSAGE + str(f1) + DOT);
        return false;
    }
    he2 = oldFace2->findHalfEdge(v1, v3);
    if ( he2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "mev",
            EDGE_MESSAGE + str(v1) + " - " + str(v3) +
            EDGE_NOT_FOUND_IN_FACE_MESSAGE + str(f2) + DOT);
        return false;
    }
    lmev(solid, he1, he2, v4, p);
    return true;
}

/**
smef: simplified version of mef operator.
*/
bool PolyhedralBoundedSolidEulerOperators::smef(
    PolyhedralBoundedSolid* solid, int f1, int v1, int v3, int newFaceId)
{
    _PolyhedralBoundedSolidFace* oldFace1;
    _PolyhedralBoundedSolidHalfEdge* he1;
    _PolyhedralBoundedSolidHalfEdge* he2;

    oldFace1 = solid->findFace(f1);
    if ( oldFace1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "smef", FACE_MESSAGE + str(f1) + NOT_FOUND_MESSAGE);
        return false;
    }
    he1 = oldFace1->findHalfEdge(v1);
    if ( he1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "smef",
            EDGE_MESSAGE + str(v1) + EDGE_NOT_FOUND_IN_FACE_WILDCARD_MESSAGE +
            str(f1) + DOT);
        return false;
    }
    he2 = oldFace1->findHalfEdge(v3);
    if ( he2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "smef",
            EDGE_MESSAGE + str(v3) + EDGE_NOT_FOUND_IN_FACE_WILDCARD_MESSAGE +
            str(f1) + DOT);
        return false;
    }
    lmef(solid, he1, he2, newFaceId);
    return true;
}

/**
mef: (high level version) make edge face (face splitting operation).
Executes a lmef in half-edges `v1`-`v2`, `v3`-`v4` of faces `f1` and `f2`.
*/
bool PolyhedralBoundedSolidEulerOperators::mef(
    PolyhedralBoundedSolid* solid, int f1, int f2,
    int v1, int v2, int v3, int v4, int newFaceId)
{
    _PolyhedralBoundedSolidFace* oldFace1;
    _PolyhedralBoundedSolidFace* oldFace2;
    _PolyhedralBoundedSolidHalfEdge* he1;
    _PolyhedralBoundedSolidHalfEdge* he2;

    oldFace1 = solid->findFace(f1);
    if ( oldFace1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "mef", FACE_MESSAGE + str(f1) + NOT_FOUND_MESSAGE);
        return false;
    }
    oldFace2 = solid->findFace(f2);
    if ( oldFace2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "mef", FACE_MESSAGE + str(f2) + NOT_FOUND_MESSAGE);
        return false;
    }
    he1 = oldFace1->findHalfEdge(v1, v2);
    if ( he1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "mef",
            EDGE_MESSAGE + str(v1) + " - " + str(v2) +
            EDGE_NOT_FOUND_IN_FACE_MESSAGE + str(f1) + DOT);
        return false;
    }
    he2 = oldFace2->findHalfEdge(v3, v4);
    if ( he2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "mef",
            EDGE_MESSAGE + str(v3) + " - " + str(v4) +
            EDGE_NOT_FOUND_IN_FACE_MESSAGE + str(f2) + DOT);
        return false;
    }
    lmef(solid, he1, he2, newFaceId);
    return true;
}

/**
kemr: (high level version) kill edge make ring (loop splitting operation).
*/
bool PolyhedralBoundedSolidEulerOperators::kemr(
    PolyhedralBoundedSolid* solid, int f1, int f2,
    int v1, int v2, int v3, int v4)
{
    _PolyhedralBoundedSolidFace* oldFace1;
    _PolyhedralBoundedSolidFace* oldFace2;
    _PolyhedralBoundedSolidHalfEdge* he1;
    _PolyhedralBoundedSolidHalfEdge* he2;

    oldFace1 = solid->findFace(f1);
    if ( oldFace1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "kemr", FACE_MESSAGE + str(f1) + NOT_FOUND_MESSAGE);
        return false;
    }
    oldFace2 = solid->findFace(f2);
    if ( oldFace2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "kemr", FACE_MESSAGE + str(f2) + NOT_FOUND_MESSAGE);
        return false;
    }
    he1 = oldFace1->findHalfEdge(v1, v2);
    if ( he1 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "kemr",
            EDGE_MESSAGE + str(v1) + " - " + str(v2) +
            EDGE_NOT_FOUND_IN_FACE_MESSAGE + str(f1) + DOT);
        return false;
    }
    he2 = oldFace2->findHalfEdge(v3, v4);
    if ( he2 == 0 ) {
        PolyhedralBoundedSolidStatistics::recordOperationFailureCase();
        report(Logger::WARNING, "kemr",
            EDGE_MESSAGE + str(v3) + " - " + str(v4) +
            EDGE_NOT_FOUND_IN_FACE_MESSAGE + str(f2) + DOT);
        return false;
    }
    lkemr(solid, he1, he2);
    return true;
}

/**
kfmrh: (high level version) KillFaceMakeRingHole, for the same-shell case
of section [MANT1988].9.2.4.
*/
bool PolyhedralBoundedSolidEulerOperators::kfmrh(PolyhedralBoundedSolid* solid, int f1, int f2)
{
    _PolyhedralBoundedSolidFace* oldFace1;
    _PolyhedralBoundedSolidFace* oldFace2;

    oldFace1 = solid->findFace(f1);
    if ( oldFace1 == 0 ) {
        report(Logger::WARNING, "kfmrh", FACE_MESSAGE + str(f1) + NOT_FOUND_MESSAGE);
        return false;
    }
    oldFace2 = solid->findFace(f2);
    if ( oldFace2 == 0 ) {
        report(Logger::WARNING, "kfmrh", FACE_MESSAGE + str(f2) + NOT_FOUND_MESSAGE);
        return false;
    }
    lkfmrh(solid, oldFace1, oldFace2);
    return true;
}

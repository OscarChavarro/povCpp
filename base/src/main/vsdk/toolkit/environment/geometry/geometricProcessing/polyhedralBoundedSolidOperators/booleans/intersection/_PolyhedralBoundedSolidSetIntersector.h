//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#ifndef ___POLYHEDRAL_BOUNDED_SOLID_SET_INTERSECTOR__
#define ___POLYHEDRAL_BOUNDED_SOLID_SET_INTERSECTOR__

#include "java/util/ArrayList.h"

#include "java/lang/String.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/_PolyhedralBoundedSolidOperator.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetOperatorVertexFace.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetOperatorVertexVertex.h"

class PolyhedralBoundedSolid;
class _PolyhedralBoundedSolidEdge;
class _PolyhedralBoundedSolidFace;
class _PolyhedralBoundedSolidHalfEdge;
class _PolyhedralBoundedSolidVertex;

/**
Helper for set operation big phase 0: generation of vertex/face and
vertex/vertex intersections, following the initial detection phase from
program [MANT1988].15.2.

C++ counterpart of Java's `_PolyhedralBoundedSolidSetIntersector`.
*/
class _PolyhedralBoundedSolidSetIntersector : public _PolyhedralBoundedSolidOperator {
public:
    typedef java::ArrayList<_PolyhedralBoundedSolidSetOperatorVertexVertex> VertexVertexList;
    typedef java::ArrayList<_PolyhedralBoundedSolidSetOperatorVertexFace> VertexFaceList;

    /** Diagnostic trace: one entry per vertex created during intersection.
    Cleared at the start of each `setOpGenerate` call. */
    static java::ArrayList<java::String> intersectionTrace;

    /**
    Coincidences found by `setOpGenerate`.
    */
    class GenerationResult {
    private:
        VertexVertexList sonvvList;
        VertexFaceList sonvaList;
        VertexFaceList sonvbList;

    public:
        GenerationResult(const VertexVertexList& sonvv,
                         const VertexFaceList& sonva,
                         const VertexFaceList& sonvb)
            : sonvvList(sonvv), sonvaList(sonva), sonvbList(sonvb) {}

        const VertexVertexList& sonvv() const { return sonvvList; }
        const VertexFaceList& sonva() const { return sonvaList; }
        const VertexFaceList& sonvb() const { return sonvbList; }
    };

    /**
    Initial vertex intersection detector for the set operations algorithm
    (big phase 0).
    Following program [MANT1988].15.2.
    */
    static GenerationResult setOpGenerate(PolyhedralBoundedSolid* inSolidA,
                                          PolyhedralBoundedSolid* inSolidB);

private:
    /**
    Point of a face boundary near a given point: on a half edge (`halfEdge`
    set) or at a vertex (`vertex` set). Java returns null for no hit.
    */
    struct BoundaryHit {
        bool found;
        _PolyhedralBoundedSolidHalfEdge* halfEdge;
        _PolyhedralBoundedSolidVertex* vertex;
        Vector3Dd point;

        BoundaryHit() : found(false), halfEdge(nullptr), vertex(nullptr) {}
        BoundaryHit(_PolyhedralBoundedSolidHalfEdge* halfEdge,
                    _PolyhedralBoundedSolidVertex* vertex,
                    const Vector3Dd& point)
            : found(true), halfEdge(halfEdge), vertex(vertex), point(point) {}
    };

    static int compareToZero(double value);
    static bool isZero(double value);
    static bool isZeroBig(double value);
    static BoundaryHit findNearbyBoundaryHit(_PolyhedralBoundedSolidFace* face,
                                             const Vector3Dd& point);
    static BoundaryHit findNearbyBoundaryHit(_PolyhedralBoundedSolidHalfEdge* halfEdge,
                                             const Vector3Dd& point,
                                             double tolerance);
    static int nextVertexId(PolyhedralBoundedSolid* current,
                            PolyhedralBoundedSolid* other);
    static void addsovf(_PolyhedralBoundedSolidHalfEdge* he,
                        _PolyhedralBoundedSolidFace* f, int BvsA,
                        VertexFaceList& sonva, VertexFaceList& sonvb);
    static void addsovv(_PolyhedralBoundedSolidVertex* a,
                        _PolyhedralBoundedSolidVertex* b, int BvsA,
                        VertexVertexList& sonvv);
    static void doVertexOnFace(_PolyhedralBoundedSolidVertex* v,
                               _PolyhedralBoundedSolidFace* f,
                               int BvsA,
                               PolyhedralBoundedSolid* edgeSolid,
                               PolyhedralBoundedSolid* faceSolid,
                               VertexVertexList& sonvv,
                               VertexFaceList& sonva,
                               VertexFaceList& sonvb);
    static _PolyhedralBoundedSolidEdge* doSetOpGenerate(
        _PolyhedralBoundedSolidEdge* e,
        _PolyhedralBoundedSolidFace* f,
        int BvsA,
        PolyhedralBoundedSolid* edgeSolid,
        PolyhedralBoundedSolid* faceSolid,
        VertexVertexList& sonvv,
        VertexFaceList& sonva,
        VertexFaceList& sonvb);
    static void processEdge(_PolyhedralBoundedSolidEdge* e,
                            PolyhedralBoundedSolid* edgeSolid,
                            PolyhedralBoundedSolid* faceSolid,
                            int BvsA,
                            VertexVertexList& sonvv,
                            VertexFaceList& sonva,
                            VertexFaceList& sonvb);
};

#endif

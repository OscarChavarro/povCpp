//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#ifndef ___POLYHEDRAL_BOUNDED_SOLID_SET_VERTEX_FACE_CLASSIFIER__
#define ___POLYHEDRAL_BOUNDED_SOLID_SET_VERTEX_FACE_CLASSIFIER__

#include "java/util/ArrayList.h"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/_PolyhedralBoundedSolidOperator.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetOperatorSectorClassificationOnFace.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/topology/_PolyhedralBoundedSolidSetOperatorNullEdge.h"

class InfinitePlane;
class PolyhedralBoundedSolid;
class _PolyhedralBoundedSolidFace;
class _PolyhedralBoundedSolidHalfEdge;
class _PolyhedralBoundedSolidVertex;

/**
Classification stages for vertex/face coincidences during set operations,
starting from the splitting-classifier rules of [MANT1988].14.5 and adapted
to boundary classification by [MANT1988].15.6.1 and problem [MANT1988].15.4.

C++ counterpart of Java's `_PolyhedralBoundedSolidSetVertexFaceClassifier`.
*/
class _PolyhedralBoundedSolidSetVertexFaceClassifier
    : public _PolyhedralBoundedSolidOperator {
public:
    typedef _PolyhedralBoundedSolidSetOperatorSectorClassificationOnFace SectorOnFace;
    typedef java::ArrayList<_PolyhedralBoundedSolidSetOperatorNullEdge> NullEdgeList;

    _PolyhedralBoundedSolidSetVertexFaceClassifier();

    /**
    Vertex/Face classifier for the set operations algorithm (big phase 1).
    Answer to problem [MANT1988].15.4.
    @param inSonea null edges of A (referenced while classifying)
    @param inSoneb null edges of B (referenced while classifying)
    */
    void classify(_PolyhedralBoundedSolidVertex* v,
                  _PolyhedralBoundedSolidFace* f,
                  int op,
                  int BvsA,
                  int flags,
                  NullEdgeList* inSonea,
                  NullEdgeList* inSoneb,
                  PolyhedralBoundedSolid* inSolidA,
                  PolyhedralBoundedSolid* inSolidB);

private:
    static int debugFlags;
    NullEdgeList* sonea;
    NullEdgeList* soneb;

    static int compareToZero(double value);
    static void applyCoplanarRulesToVertexFaceNeighborhood(
        java::ArrayList<SectorOnFace>& nbr,
        _PolyhedralBoundedSolidFace* referenceFace,
        InfinitePlane* referencePlane,
        int BvsA,
        int op,
        bool useMirrorFace);
    static int nextVertexId(PolyhedralBoundedSolid* current,
                            PolyhedralBoundedSolid* other);
    static Vector3Dd inside(_PolyhedralBoundedSolidHalfEdge* he);
    static java::ArrayList<SectorOnFace> vertexFaceGetNeighborhood(
        _PolyhedralBoundedSolidVertex* vtx,
        InfinitePlane* referencePlane,
        int BvsA);
    static void vertexFaceReclassifyOnSectors(
        java::ArrayList<SectorOnFace>& nbr,
        _PolyhedralBoundedSolidFace* referenceFace,
        InfinitePlane* referencePlane,
        int BvsA,
        int op);
    static void printNbr(const java::ArrayList<SectorOnFace>& neighborSectorsInfo);
    static bool inplaneEdgesOn(const java::ArrayList<SectorOnFace>& nbr);
    static void vertexFaceReclassifyOnEdges(java::ArrayList<SectorOnFace>& nbr,
                                            int op);
    void vertexFaceInsertNullEdges(java::ArrayList<SectorOnFace>& nbr,
                                   _PolyhedralBoundedSolidFace* f,
                                   _PolyhedralBoundedSolidVertex* v,
                                   int BvsA,
                                   PolyhedralBoundedSolid* inSolidA,
                                   PolyhedralBoundedSolid* inSolidB);
    void vertexFaceClassify(_PolyhedralBoundedSolidVertex* v,
                            _PolyhedralBoundedSolidFace* f,
                            int op,
                            int BvsA,
                            PolyhedralBoundedSolid* inSolidA,
                            PolyhedralBoundedSolid* inSolidB);
    void makeRing(_PolyhedralBoundedSolidFace* f,
                  _PolyhedralBoundedSolidVertex* v,
                  int type,
                  PolyhedralBoundedSolid* inSolidA,
                  PolyhedralBoundedSolid* inSolidB);
};

#endif

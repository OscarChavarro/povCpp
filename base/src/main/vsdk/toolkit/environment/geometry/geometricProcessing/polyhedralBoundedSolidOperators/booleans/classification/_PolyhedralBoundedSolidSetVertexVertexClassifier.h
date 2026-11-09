//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#ifndef ___POLYHEDRAL_BOUNDED_SOLID_SET_VERTEX_VERTEX_CLASSIFIER__
#define ___POLYHEDRAL_BOUNDED_SOLID_SET_VERTEX_VERTEX_CLASSIFIER__

#include "java/util/ArrayList.h"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/_PolyhedralBoundedSolidOperator.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetOperatorSectorClassificationOnSector.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetOperatorSectorClassificationOnVertex.h"

class _PolyhedralBoundedSolidVertex;

/**
Geometric preprocessing and sector reclassification for vertex/vertex matches,
following section [MANT1988].15.6.2 and programs [MANT1988].15.7 through
[MANT1988].15.10.

C++ counterpart of Java's `_PolyhedralBoundedSolidSetVertexVertexClassifier`.
*/
class _PolyhedralBoundedSolidSetVertexVertexClassifier
    : public _PolyhedralBoundedSolidOperator {
public:
    typedef _PolyhedralBoundedSolidSetOperatorSectorClassificationOnVertex SectorOnVertex;
    typedef _PolyhedralBoundedSolidSetOperatorSectorClassificationOnSector SectorOnSector;

    /**
    Neighborhoods of the two vertices and their intersecting sector pairs.
    */
    class VertexVertexClassificationData {
    public:
        java::ArrayList<SectorOnVertex> nba;
        java::ArrayList<SectorOnVertex> nbb;
        java::ArrayList<SectorOnSector> sectors;

        VertexVertexClassificationData(const java::ArrayList<SectorOnVertex>& inNba,
                                       const java::ArrayList<SectorOnVertex>& inNbb,
                                       const java::ArrayList<SectorOnSector>& inSectors)
            : nba(inNba), nbb(inNbb), sectors(inSectors) {}
    };

    /**
    Vertex/Vertex classifier for the set operations algorithm (big phase 2).
    Following program [MANT1988].15.6. Similar in structure to program
    [MANT1988].14.3.
    */
    VertexVertexClassificationData classify(_PolyhedralBoundedSolidVertex* va,
                                            _PolyhedralBoundedSolidVertex* vb,
                                            int op,
                                            int flags);

private:
    static int debugFlags;
    java::ArrayList<SectorOnVertex> nba;
    java::ArrayList<SectorOnVertex> nbb;
    java::ArrayList<SectorOnSector> sectors;

    /**
    @return 1 (same orientation), 0 (opposite) or -1 (not coplanar or not
    computable; Java returns null)
    */
    int coplanarSameOrientationForSectorPair(int secta, int sectb);
    static java::ArrayList<SectorOnVertex> nbrpreproc(_PolyhedralBoundedSolidVertex* v);
    static bool sectoroverlap(const SectorOnVertex& na, const SectorOnVertex& nb);
    static bool sctrwitthin(const Vector3Dd& dir, const Vector3Dd& ref1,
                            const Vector3Dd& ref2, const Vector3Dd& ref12);
    static int compareToZero(double value);
    static int resolveCoplanarVertexVertexClass(int op, bool sameOrientation,
                                                bool sideA);
    bool vertexVertexSectorIntersectionTest(int i, int j);
    void vertexVertexGetNeighborhood(_PolyhedralBoundedSolidVertex* va,
                                     _PolyhedralBoundedSolidVertex* vb);
    void vertexVertexReclassifyOnSectors(int op);
    void vertexVertexReclassifyOnEdges(int op);
};

#endif

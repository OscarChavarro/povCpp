//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#ifndef ___POLYHEDRAL_BOUNDED_SOLID_SPLITTER__
#define ___POLYHEDRAL_BOUNDED_SOLID_SPLITTER__


#include "java/util/Collections.h"
#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/_PolyhedralBoundedSolidOperator.h"

class InfinitePlane;
class PolyhedralBoundedSolid;
class _PolyhedralBoundedSolidEdge;
class _PolyhedralBoundedSolidFace;
class _PolyhedralBoundedSolidHalfEdge;
class _PolyhedralBoundedSolidVertex;

/**
This class is used to store vertex / halfedge neigborhood information, as presented
in section [MANT1988].14.5, and program [MANT1988].14.3.
*/
class _PolyhedralBoundedSolidSplitterSectorClassification : public _PolyhedralBoundedSolidOperator {
public:
    static const int ABOVE = 1;
    static const int BELOW = -1;
    static const int ON = 0;

    static const int COPLANAR_FACE = 10;
    static const int INPLANE_EDGE = 20;
    static const int CROSSING_EDGE = 30;
    static const int UNDEFINED = 40;

    _PolyhedralBoundedSolidHalfEdge* sector;
    int cl;

    // Following attributes are not taken from [MANT1988], and all operations
    // on them are fine tunning options aditional to original algorithm.
    bool isWide;
    Vector3Dd position;
    int situation;

    _PolyhedralBoundedSolidSplitterSectorClassification();

    java::String toString() const;
};

/**
Class `_PolyhedralBoundedSolidSplitterNullEdge` plays a role of a decorator
design patern for class `_PolyhedralBoundedSolidEdge`, and adds sort-ability.
*/
class _PolyhedralBoundedSolidSplitterNullEdge : public _PolyhedralBoundedSolidOperator {
private:
    static PolyhedralBoundedSolidNumericPolicy::ToleranceContext nullEdgeNumericContext;

public:
    _PolyhedralBoundedSolidEdge* e;

    // Default constructible, as the elements of `java::ArrayList`
    _PolyhedralBoundedSolidSplitterNullEdge() : e(nullptr) {}
    explicit _PolyhedralBoundedSolidSplitterNullEdge(_PolyhedralBoundedSolidEdge* e);

    static void setNumericContext(
        const PolyhedralBoundedSolidNumericPolicy::ToleranceContext* context);

    /**
    As Java `compareTo`: orders by the position of the starting vertex of
    the right half edge. It never answers 0.
    */
    int compareTo(const _PolyhedralBoundedSolidSplitterNullEdge& other) const;
};

/**
This is a utility class containing operations for implementing the boundary
representation split methods over winged-edge data structures, as presented
at chapter [MANT1988].14.

This class offers just one public method, which is supposed to be called
from GeometricModeler class.

C++ counterpart of Java's `_PolyhedralBoundedSolidSplitter`.
*/
class _PolyhedralBoundedSolidSplitter : public _PolyhedralBoundedSolidOperator {
public:
    /**
    Given the input `inSolid` and the cutting plane `inSplittingPlane`,
    this method appends to the `outSolidsAbove` list the solids resulting
    from cutting the solid with the plane and resulting above the plane,
    similarly, `outSolidsBelow` will be appended with solid pieces
    resulting below the plane.

    Current macro-algorithm follows the strategy outlined on sections
    [MANT1988].14.1, [MANT1988].14.2 and [MANT1988].14.3 and program
    [MANT1988].14.1.

    C++ port note: the caller owns the solids appended to the lists. When
    the plane does not cut the solid, `inSolid` itself is appended to
    `outSolidsAbove` (as in Java); otherwise its faces, edges and vertices
    are moved to the new solids and `inSolid` is left empty.
    */
    static void split(PolyhedralBoundedSolid* inSolid,
                      const InfinitePlane& inSplittingPlane,
                      java::ArrayList<PolyhedralBoundedSolid*>& outSolidsAbove,
                      java::ArrayList<PolyhedralBoundedSolid*>& outSolidsBelow);

private:
    typedef _PolyhedralBoundedSolidSplitterSectorClassification SectorClassification;

    /**
    Following variable `soov` ("set of ON-vertices") from program [MANT1988].14.1.
    */
    static java::ArrayList<_PolyhedralBoundedSolidVertex*> soov;

    /**
    Following variable `sone` ("set of null edges") from program [MANT1988].14.1.
    */
    static java::ArrayList<_PolyhedralBoundedSolidSplitterNullEdge> sone;

    /**
    Following variable `sonf` ("set of null faces") from program [MANT1988].14.1.
    */
    static java::ArrayList<_PolyhedralBoundedSolidFace*> sonf;

    static java::ArrayList<_PolyhedralBoundedSolidFace*> facesToFixAbove;
    static java::ArrayList<_PolyhedralBoundedSolidFace*> facesToFixBelow;

    /**
    Following variable `ends` from program [MANT1988].14.9.
    */
    static java::ArrayList<_PolyhedralBoundedSolidHalfEdge*> ends;
    static java::ArrayList<_PolyhedralBoundedSolidHalfEdge*> tieds;

    static int compareToZero(double value);
    static void addsoov(_PolyhedralBoundedSolidVertex* v);
    static void splitGenerate(PolyhedralBoundedSolid* inSolid,
                              const InfinitePlane& inSplittingPlane);
    static java::ArrayList<SectorClassification> getNeighborhood(_PolyhedralBoundedSolidVertex* vtx, const InfinitePlane& inSplittingPlane);
    static bool checkSplitterSectorWideness(_PolyhedralBoundedSolidHalfEdge* he);
    static bool inplaneEdgesOn(const java::ArrayList<SectorClassification>& nbr);
    static void reclassifyOnSectors(java::ArrayList<SectorClassification>& nbr,
                                    const InfinitePlane& inSplittingPlane);
    static void reclassifyOnEdges(java::ArrayList<SectorClassification>& nbr);
    static void insertNullEdges(java::ArrayList<SectorClassification>& nbr,
                                PolyhedralBoundedSolid* inSolid,
                                const InfinitePlane& inSplittingPlane);
    static void splitClassify(PolyhedralBoundedSolid* inSolid,
                              const InfinitePlane& inSplittingPlane);
    static _PolyhedralBoundedSolidHalfEdge* canJoin(_PolyhedralBoundedSolidHalfEdge* he);
    static void printNbr(const java::ArrayList<SectorClassification>& neighborSectorsInfo);
    static void printEnds();
    static void cut(_PolyhedralBoundedSolidHalfEdge* he);
    static bool isLoose(_PolyhedralBoundedSolidHalfEdge* he);
    static void splitConnect();
    static void classify(PolyhedralBoundedSolid* s,
                         PolyhedralBoundedSolid* above,
                         PolyhedralBoundedSolid* below);
    static bool isNullFace(_PolyhedralBoundedSolidFace* f);
    static void destroy(PolyhedralBoundedSolid* inSolid);
    static bool faceInsideFace(_PolyhedralBoundedSolidFace* a,
                               _PolyhedralBoundedSolidFace* b);
    static void fixNullFaces(java::ArrayList<_PolyhedralBoundedSolidFace*>& l);
    static void splitFinish(PolyhedralBoundedSolid* inSolid,
                            java::ArrayList<PolyhedralBoundedSolid*>& outSolidsAbove,
                            java::ArrayList<PolyhedralBoundedSolid*>& outSolidsBelow);
};

#endif

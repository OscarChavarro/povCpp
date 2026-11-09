#ifndef ___POLYHEDRAL_BOUNDED_SOLID_AXIS_ALIGNED_CELL_FALLBACK__
#define ___POLYHEDRAL_BOUNDED_SOLID_AXIS_ALIGNED_CELL_FALLBACK__

#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "java/util/TreeMap.h"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/_PolyhedralBoundedSolidOperator.h"

class PolyhedralBoundedSolid;
class _PolyhedralBoundedSolidEdge;
class _PolyhedralBoundedSolidHalfEdge;
class _PolyhedralBoundedSolidVertex;

/**
Structural-shape boolean fallback for axis-aligned cell decompositions: when
both operands are axis-aligned, classifies the cell grid against each operand
and rebuilds the boolean result cell-by-cell.

C++ counterpart of Java's `_PolyhedralBoundedSolidAxisAlignedCellFallback`.
*/
class _PolyhedralBoundedSolidAxisAlignedCellFallback
    : public _PolyhedralBoundedSolidOperator {
public:
    static bool axisAlignedCellSelected(bool insideA, bool insideB, int op);

    /**
    @return a new solid owned by the caller, or null when the operands are
    not axis-aligned or the grid is too large
    */
    static PolyhedralBoundedSolid* buildAxisAlignedCellBooleanFallback(
        PolyhedralBoundedSolid* inSolidA,
        PolyhedralBoundedSolid* inSolidB,
        int op);

private:
    class AxisAlignedCellBooleanBuilder {
    public:
        AxisAlignedCellBooleanBuilder(const java::ArrayList<double>& xCoordinates,
            const java::ArrayList<double>& yCoordinates,
            const java::ArrayList<double>& zCoordinates);
        void addQuad(const int corners[4][3]);
        PolyhedralBoundedSolid* result();
    private:
        PolyhedralBoundedSolid* solid;
        java::ArrayList<double> xCoordinates;
        java::ArrayList<double> yCoordinates;
        java::ArrayList<double> zCoordinates;
        java::TreeMap<java::String, _PolyhedralBoundedSolidVertex*> vertices;
        java::TreeMap<java::String, _PolyhedralBoundedSolidEdge*> edges;
        int nextVertexId;
        int nextFaceId;

        static java::String vertexKey(int ix, int iy, int iz);
        _PolyhedralBoundedSolidVertex* vertexAt(int ix, int iy, int iz);
        static java::String edgeKey(_PolyhedralBoundedSolidVertex* a,
            _PolyhedralBoundedSolidVertex* b);
        void attachEdge(_PolyhedralBoundedSolidHalfEdge* he,
            _PolyhedralBoundedSolidVertex* a,
            _PolyhedralBoundedSolidVertex* b);
    };

    _PolyhedralBoundedSolidAxisAlignedCellFallback();
    static bool isAxisAlignedEdge(_PolyhedralBoundedSolidEdge* edge);
    static bool isAxisAlignedSolid(PolyhedralBoundedSolid* solid);
    static int classifyPointForAxisAlignedFallback(PolyhedralBoundedSolid* solid,
        const Vector3Dd& point);
    static void addAxisAlignedBoundaryQuad(AxisAlignedCellBooleanBuilder& builder,
        int axis, bool positiveSide, int ix, int iy, int iz);
    static java::ArrayList<double> uniformCoordinates(double min, double max,
        int divisions);
    static PolyhedralBoundedSolid* buildUniformSampledCellBooleanFallback(
        PolyhedralBoundedSolid* inSolidA,
        PolyhedralBoundedSolid* inSolidB,
        int op);
    static void addOccupiedCellQuads(AxisAlignedCellBooleanBuilder& builder,
        const java::ArrayList<bool>& occupied, int nx, int ny, int nz);
};

#endif

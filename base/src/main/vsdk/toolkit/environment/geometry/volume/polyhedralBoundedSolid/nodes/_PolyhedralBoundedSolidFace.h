#ifndef __POLYHEDRAL_BOUNDED_SOLID_FACE__
#define __POLYHEDRAL_BOUNDED_SOLID_FACE__

#include "java/lang/String.h"

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
class PolyhedralBoundedSolid;
class InfinitePlane;
class _PolyhedralBoundedSolidLoop;
class _PolyhedralBoundedSolidHalfEdge;
class _PolyhedralBoundedSolidVertex;

class _PolyhedralBoundedSolidFace {
public:
    class PointInsideResult {
    private:
        int status_;
        _PolyhedralBoundedSolidHalfEdge* intersectedHalfedge_;
        _PolyhedralBoundedSolidVertex* intersectedVertex_;

    public:
        PointInsideResult(
            int status,
            _PolyhedralBoundedSolidHalfEdge* intersectedHalfedge,
            _PolyhedralBoundedSolidVertex* intersectedVertex)
            : status_(status),
              intersectedHalfedge_(intersectedHalfedge),
              intersectedVertex_(intersectedVertex)
        {
        }

        int status() const { return status_; }
        _PolyhedralBoundedSolidHalfEdge* intersectedHalfedge() const
        {
            return intersectedHalfedge_;
        }
        _PolyhedralBoundedSolidVertex* intersectedVertex() const
        {
            return intersectedVertex_;
        }
    };

    int id;
    PolyhedralBoundedSolid* parentSolid;
    java::ArrayList<_PolyhedralBoundedSolidLoop*> boundariesList;

    _PolyhedralBoundedSolidFace(PolyhedralBoundedSolid* parent, int id);
    ~_PolyhedralBoundedSolidFace();

    _PolyhedralBoundedSolidHalfEdge* findHalfEdge(int vn1, int vn2);
    _PolyhedralBoundedSolidHalfEdge* findHalfEdge(int vn1);
    bool calculatePlane();
    InfinitePlane* getContainingPlane();
    int testPointInside(const Vector3Dd& point, double tolerance);
    int testPointInside(
        const Vector3Dd& point,
        double tolerance,
        InfinitePlane* plane);
    PointInsideResult testPointInsideDetailed(
        const Vector3Dd& point,
        double tolerance);
    PointInsideResult testPointInsideDetailed(
        const Vector3Dd& point,
        double tolerance,
        InfinitePlane* plane);
    void revert();

    /**
    @return a description of the node, as Java `toString`
    */
    java::String toString() const;

private:
    static double boundaryLoopAreaMagnitude(_PolyhedralBoundedSolidLoop* loop);
    static _PolyhedralBoundedSolidLoop* selectLoopForPlaneCalculation(
        java::ArrayList<_PolyhedralBoundedSolidLoop*>& boundariesList);
    static InfinitePlane* calculatePlaneByNewell(
        java::ArrayList<_PolyhedralBoundedSolidLoop*>& boundariesList,
        double tolerance);
    InfinitePlane* calculatePlaneByCorner(double tolerance);
    Vector3Dd dropCoordinate(const Vector3Dd& in, int coord) const;
};

#endif

#ifndef __POLYHEDRAL_BOUNDED_SOLID_TOPOLOGICAL_VALIDATOR__
#define __POLYHEDRAL_BOUNDED_SOLID_TOPOLOGICAL_VALIDATOR__

class PolyhedralBoundedSolid;

/**
Topological validation helpers for the half-edge representation described in
[MANT1988].10.2.1 and [MANT1988].10.2.2.
*/
class _PolyhedralBoundedSolidTopologicalValidator {
public:
    /**
    Checks the fundamental half-edge consistency expected from the graph and
    identification structure described in [MANT1988].10.2.1 and
    [MANT1988].10.2.2.
    @param solid solid to check
    @return true when every edge is used exactly by two half-edges of closed
    loops
    */
    static bool validateTopologicalIntegrity(PolyhedralBoundedSolid* solid);

    /**
    Rebuilds the vertex-to-emanating-halfedge links required by the vertex
    node definition of [MANT1988].10.2.1 and used throughout the
    Euler-operator programs of chapter [MANT1988].11. Vertices not reached by
    any half-edge are removed from the solid vertex list.
    @param solid solid to update
    */
    static void remakeEmanatingHalfedgesReferences(PolyhedralBoundedSolid* solid);

private:
    _PolyhedralBoundedSolidTopologicalValidator();
};

#endif

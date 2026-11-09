//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#ifndef ___POLYHEDRAL_BOUNDED_SOLID_ID_NAMESPACE__
#define ___POLYHEDRAL_BOUNDED_SOLID_ID_NAMESPACE__

class PolyhedralBoundedSolid;

/**
Centralizes face-ID and vertex-ID allocation for the duration of a single
boolean set-operation.  Replaces the scattered ad-hoc pattern
`max(A.getMaxVertexId(), B.getMaxVertexId()) + 1` used by the
intersector and the connector, which produced duplicate IDs when called
multiple times without updating both solids' stored maxima.

Usage: construct once per `setOp` call after `updmaxnames`, then inject
into the Intersector and Finisher so that every vertex or face created
during the pipeline consumes a globally unique ID.

Traceability: [MANT1988] §13.1 `getmaxnames` / `updmaxnames`.

C++ counterpart of Java's `_PolyhedralBoundedSolidIdNamespace`.
*/
class _PolyhedralBoundedSolidIdNamespace {
private:
    int nextVertexIdValue;
    int nextFaceIdValue;

public:
    /**
    Initialises the namespace from the current maxima of both operands.
    Must be called after `updmaxnames` so that the IDs of solidB have
    already been offset past solidA.
    @param solidA first boolean operand.
    @param solidB second boolean operand (IDs already offset by updmaxnames).
    */
    _PolyhedralBoundedSolidIdNamespace(PolyhedralBoundedSolid* solidA,
                                       PolyhedralBoundedSolid* solidB);

    /**
    Returns the next available vertex ID and advances the counter.
    Also updates both solids' stored maximum so that any subsequent Euler
    operator call sees a consistent state.
    @param solidA first operand to keep in sync.
    @param solidB second operand to keep in sync.
    @return a vertex ID that is unique within this pipeline invocation.
    */
    int nextVertexId(PolyhedralBoundedSolid* solidA,
                     PolyhedralBoundedSolid* solidB);

    /**
    Returns the next available face ID and advances the counter.
    Also updates both solids' stored maximum.
    @param solidA first operand to keep in sync.
    @param solidB second operand to keep in sync.
    @return a face ID that is unique within this pipeline invocation.
    */
    int nextFaceId(PolyhedralBoundedSolid* solidA,
                   PolyhedralBoundedSolid* solidB);

    /**
    Returns the next available face ID using only one solid's context — for
    use in the Finisher where the result solid is independent of the two
    original operands.
    @param solid the result solid to keep in sync.
    @return a face ID unique within this pipeline invocation.
    */
    int nextFaceId(PolyhedralBoundedSolid* solid);

    /**
    @return the ID that the next `nextVertexId(...)` call would return.
    */
    int peekNextVertexId() const;

    /**
    @return the ID that the next `nextFaceId(...)` call would return.
    */
    int peekNextFaceId() const;

    /**
    Procedure `updmaxnames` functionality is described on section
    [MANT1988].15.4. Increments the face and vertex identifiers of
    `solidToUpdate` so that they do not overlap with `referenceSolid`
    identifiers.
    @param solidToUpdate solid whose IDs are offset past the reference.
    @param referenceSolid solid whose maxima define the offset.
    */
    static void updmaxnames(PolyhedralBoundedSolid* solidToUpdate,
                            PolyhedralBoundedSolid* referenceSolid);

    /**
    Resolves the next vertex ID through the given namespace when available,
    falling back to the legacy `max(maxVertexId)+1` policy when no
    namespace is active.
    @param current first operand.
    @param other second operand.
    @param ns active namespace, or null to use the fallback.
    @return a vertex ID unique within the current pipeline invocation.
    */
    static int nextVertexId(PolyhedralBoundedSolid* current,
                            PolyhedralBoundedSolid* other,
                            _PolyhedralBoundedSolidIdNamespace* ns);
};

#endif

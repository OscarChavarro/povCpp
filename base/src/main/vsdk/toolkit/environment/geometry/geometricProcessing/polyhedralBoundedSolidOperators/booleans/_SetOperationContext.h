#ifndef ___SET_OPERATION_CONTEXT__
#define ___SET_OPERATION_CONTEXT__

#include "java/util/ArrayList.h"

#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetOperatorVertexFace.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetOperatorVertexVertex.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/topology/_PolyhedralBoundedSolidSetOperatorNullEdge.h"

class _PolyhedralBoundedSolidFace;

/**
Per-invocation mutable state for one boolean set-operation: the Mantyla
"son*" globals from program [MANT1988].15.1 (sonvv, sonva, sonvb, sonea,
soneb, sonfa, sonfb). One instance is threaded explicitly through the
pipeline (generate -> classify -> connect -> finish) so the lists are not
shared static fields.

The numeric tolerance context and the id namespace are intentionally left as
managed static state in `_PolyhedralBoundedSolidOperator`; only the
per-operation list state is carried here.

C++ counterpart of Java's `_SetOperationContext`.
*/
class _SetOperationContext {
public:
    /** Following variable `sonvv` from program [MANT1988].15.1. */
    java::ArrayList<_PolyhedralBoundedSolidSetOperatorVertexVertex> sonvv;

    /** Following variable `sonva` from program [MANT1988].15.1. */
    java::ArrayList<_PolyhedralBoundedSolidSetOperatorVertexFace> sonva;

    /** Following variable `sonvb` from program [MANT1988].15.1. */
    java::ArrayList<_PolyhedralBoundedSolidSetOperatorVertexFace> sonvb;

    /** Following variable `sonea` from program [MANT1988].15.1. */
    java::ArrayList<_PolyhedralBoundedSolidSetOperatorNullEdge> sonea;

    /** Following variable `soneb` from program [MANT1988].15.1. */
    java::ArrayList<_PolyhedralBoundedSolidSetOperatorNullEdge> soneb;

    /** Following variable `sonfa` from program [MANT1988].15.1. */
    java::ArrayList<_PolyhedralBoundedSolidFace*> sonfa;

    /** Following variable `sonfb` from program [MANT1988].15.1. */
    java::ArrayList<_PolyhedralBoundedSolidFace*> sonfb;
};

#endif

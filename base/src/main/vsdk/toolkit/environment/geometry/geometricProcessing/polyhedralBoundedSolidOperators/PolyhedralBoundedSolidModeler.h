#ifndef __POLYHEDRAL_BOUNDED_SOLID_MODELER__
#define __POLYHEDRAL_BOUNDED_SOLID_MODELER__

#include "java/lang/IllegalStateException.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/processing/ProcessingElement.h"

#include "java/util/ArrayList.h"

class InfinitePlane;
class ParametricCurve;
class PolyhedralBoundedSolid;
class _PolyhedralBoundedSolidFace;

/**
Utility class with static modeling and boolean operations specific to
`PolyhedralBoundedSolid`.

This class contains creation/sweep/split/set-op helpers and centralizes the
polyhedral B-Rep operations previously exposed through `GeometricModeler`.

C++ port note: `setOp` returns a new solid owned by the caller; the
operands are consumed (modified) but stay owned by the caller, see
`_PolyhedralBoundedSolidSetOperator::setOp`.
*/
class PolyhedralBoundedSolidModeler : public ProcessingElement {
public:
    static const int UNION = 1;
    static const int INTERSECTION = 2;
    static const int SUBTRACT = 3;

    /**
    Applies a transformation matrix to all vertices in the solid.
    @param solid target solid instance.
    @param transformation transformation matrix to apply.
    */
    static void applyTransformation(PolyhedralBoundedSolid* solid,
                                    const Matrix4x4d& transformation);

    /**
    Implements the construction style from [MANT1988] section 12.2 / program
    12.1.

    Builds an arc on plane `z = height`, centered at (`cx`, `cy`), radius
    `radius`. The start vertex already exists in `faceId` and is identified
    by `vertexId`; the method appends `n` new edge steps from `phi1` to
    `phi2` (degrees, counterclockwise, 0 degrees on +X).
    */
    static void addArcToExistingFace(PolyhedralBoundedSolid* solid,
        int faceId, int vertexId, double cx, double cy, double radius,
        double height, double phi1, double phi2, int n);

    /**
    Implements [MANT1988] section 12.2 / program 12.2.

    Creates a planar circular lamina as a single face by:
    1) creating an initial vertex (`mvfs`)
    2) adding arc vertices (`addArc`)
    3) closing the loop (`smef`)
    @return a new solid, owned by the caller
    */
    static PolyhedralBoundedSolid* createCircularLamina(
        double cx, double cy, double rad, double h, int n);

    /**
    Generalized translational sweep of a face, inspired by [MANT1988] 12.3.1.

    Instead of pure translation, transform `T` can include translation,
    rotation, and scale in the face-local context.

    PRE: `face` is closed and planar.
    */
    static void translationalSweepExtrudeFace(PolyhedralBoundedSolid* solid,
        _PolyhedralBoundedSolidFace* face,
        const Matrix4x4d& transformationMatrix);

    /**
    Variant of `translationalSweepExtrudeFace` with an extra planar check
    pass.

    After sweep generation, each created side face is tested for planarity.
    If a face is non-planar, it is split once (`lmef`) to triangulate it.
    */
    static void translationalSweepExtrudeFacePlanar(
        PolyhedralBoundedSolid* solid,
        _PolyhedralBoundedSolidFace* face,
        const Matrix4x4d& transformationMatrix);

    /**
    Rotational sweep of an open wire profile around the X axis, following the
    construction style of [MANT1988] 12.2 and 12.5.

    PRE:
    - `solid` is a wire-like profile (single face, open loop)
    - profile lies on `z = 0`
    - `numberOfFaces >= 3`
    */
    static void rotationalSweepExtrudeWireAroundXAxis(
        PolyhedralBoundedSolid* solid, int numberOfFaces);

    /**
    Builds a planar B-Rep face (with holes) from the contours of a curve.
    @param curve contours, separated by `BREAK` markers
    @return a new solid, owned by the caller
    */
    static PolyhedralBoundedSolid* createBrepFromParametricCurve(
        ParametricCurve* curve);

    /**
    Convenience wrapper over `_PolyhedralBoundedSolidSplitter::split`.

    Splits `inSolid` by `inSplittingPlane` and appends resulting pieces to
    `outSolidsAbove` and `outSolidsBelow` (owned by the caller; see
    `_PolyhedralBoundedSolidSplitter::split`).
    */
    static void split(PolyhedralBoundedSolid* inSolid,
                      const InfinitePlane& inSplittingPlane,
                      java::ArrayList<PolyhedralBoundedSolid*>& outSolidsAbove,
                      java::ArrayList<PolyhedralBoundedSolid*>& outSolidsBelow);

    /**
    Convenience wrapper over `_PolyhedralBoundedSolidSetOperator::setOp`
    with configurable debug mode, final face maximization and strict result
    validation (enabled by default).

    Strict validation performs global shell/Euler analysis and an
    all-face-pairs intersection scan; callers can pass false for an explicit
    performance/legacy-compatibility opt-out.

    @param inSolidA first operand (consumed, still owned by the caller)
    @param inSolidB second operand (consumed, still owned by the caller)
    @param op UNION, INTERSECTION or SUBTRACT
    @param withDebug true to dump the pipeline state
    @param maximizeResultFaces true to maximize the result faces
    @param doStrictValidation true to validate the result strictly
    @return the result, owned by the caller
    @throws java::IllegalStateException (Java IllegalStateException) when a completed
    boolean result fails the strict B-Rep postcondition
    */
    static PolyhedralBoundedSolid* setOp(
        PolyhedralBoundedSolid* inSolidA,
        PolyhedralBoundedSolid* inSolidB,
        int op,
        bool withDebug = false,
        bool maximizeResultFaces = true,
        bool doStrictValidation = true);
};

#endif

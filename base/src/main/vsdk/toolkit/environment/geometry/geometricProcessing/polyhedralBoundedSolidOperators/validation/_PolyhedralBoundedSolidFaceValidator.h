#ifndef __POLYHEDRAL_BOUNDED_SOLID_FACE_VALIDATOR__
#define __POLYHEDRAL_BOUNDED_SOLID_FACE_VALIDATOR__

#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidNumericPolicy.h"

class _PolyhedralBoundedSolidFace;

/**
Geometric checks of single faces of a `PolyhedralBoundedSolid`, used to
detect faces that can not be presented as a surface (i.e. by the renderers).

C++ counterpart of Java's `_PolyhedralBoundedSolidFaceValidator`.
*/
class _PolyhedralBoundedSolidFaceValidator {
public:
    /**
    @param face face to check
    @return true if the face is not planar, has no area, or has two
    non-adjacent edges closer than the tolerance of the face
    */
    static bool isSurfaceDegenerate(_PolyhedralBoundedSolidFace* face);

    /**
    @param face face to measure
    @return the sum of the areas of the loops of the face
    */
    static double faceArea(_PolyhedralBoundedSolidFace* face);

    /**
    @param face face to check
    @param numericContext tolerances of the face
    @return true if two edges of the face that do not share an end point are
    closer than the tolerance
    */
    static bool hasCloseNonAdjacentEdges(
        _PolyhedralBoundedSolidFace* face,
        const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext);

private:
    _PolyhedralBoundedSolidFaceValidator() {}
};

#endif

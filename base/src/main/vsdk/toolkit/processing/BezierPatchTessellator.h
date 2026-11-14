#ifndef VSDK_BEZIER_PATCH_TESSELLATOR_H
#define VSDK_BEZIER_PATCH_TESSELLATOR_H

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
class SubdividedBezierPatch;
class TriangleMesh;

// Drawable triangle mesh of the Bezier surface of a patch's control net:
// (uSteps + 1) x (vSteps + 1) points of the exact surface at uniform (u, v),
// each with the unit normal S_u x S_v of the Bernstein derivatives (or, where
// that vanishes, the mean of its triangles' normals) and (u, v) as texture
// coordinates, joined by two triangles per parameter cell facing the u x v
// side. It approximates the Bezier surface, not the patch's own subdivision,
// which a ray tracer intersects instead: with the patch's uniform steps both
// share the grid points, while the adaptive subdivisions place their triangles
// elsewhere.
class BezierPatchTessellator {
private:
    // Cubic Bernstein polynomial k at t and its derivative.
    static double bernstein(int k, double t);
    static double bernsteinDerivative(int k, double t);
public:
    // Unit S_u x S_v at (u, v), or the zero vector where it vanishes.
    static Vector3Dd surfaceNormal(const SubdividedBezierPatch& patch, double u, double v);
    // Largest accepted steps along u or v.
    static const int MAXIMUM_STEPS = 1024;
    // Returns a new mesh owned by the caller, or null when a step count is
    // outside 1..MAXIMUM_STEPS.
    static TriangleMesh* tessellate(const SubdividedBezierPatch& patch, int uSteps, int vSteps);
};

#endif

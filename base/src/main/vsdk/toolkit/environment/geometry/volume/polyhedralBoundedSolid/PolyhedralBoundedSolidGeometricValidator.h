#ifndef __POLYHEDRAL_BOUNDED_SOLID_GEOMETRIC_VALIDATOR__
#define __POLYHEDRAL_BOUNDED_SOLID_GEOMETRIC_VALIDATOR__

#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidNumericPolicy.h"
class PolyhedralBoundedSolid;
class _PolyhedralBoundedSolidFace;
class Vector3Dd;

/**
Geometric validation helpers for polyhedral B-Reps, centered on the planar
face model of [MANT1988].10.2.1 and the geometric primitives developed in
chapter [MANT1988].13.

The `msg` parameters play the role of the Java `StringBuilder`: failure
details are appended to them, and they can be null when not needed.
*/
class PolyhedralBoundedSolidGeometricValidator {
public:
    /**
    Checks whether a set of face vertices can define the planar polygon
    required by [MANT1988].10.2.1 and by the face-equation procedure of
    [MANT1988].13.1.
    @param points face points
    @return true when the points are coplanar
    */
    static bool validateFacePointsAreCoplanar(java::ArrayList<Vector3Dd>& points);
    static bool validateFacePointsAreCoplanar(java::ArrayList<Vector3Dd>& points, const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext);

    /**
    Appends the vertex positions of every loop of `face` to `outPoints`.
    @param face face to traverse
    @param outPoints receives the points
    @return false when some loop is empty or not closed (Java returns null)
    */
    static bool extractPointsFromFace(_PolyhedralBoundedSolidFace* face, java::ArrayList<Vector3Dd>& outPoints);

    static bool validateFaceIsPlanar(_PolyhedralBoundedSolidFace* face);
    static bool validateFaceIsPlanar(_PolyhedralBoundedSolidFace* face, const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext);

    static bool validateAllFacesPlanarityAndPlanes(PolyhedralBoundedSolid* solid, java::String* msg);
    static bool validateAllFacesPlanarityAndPlanes(PolyhedralBoundedSolid* solid, const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext, java::String* msg);

    static bool validateConsistentFaceOrientations(PolyhedralBoundedSolid* solid, java::String* msg);
    static bool validateConsistentFaceOrientations(PolyhedralBoundedSolid* solid, const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext, java::String* msg);

    static bool validateLoopsStrict(PolyhedralBoundedSolid* solid, java::String* msg);
    static bool validateLoopsStrict(PolyhedralBoundedSolid* solid, const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext, java::String* msg);

    static bool validateFaceIntersectionsStrict(PolyhedralBoundedSolid* solid, java::String* msg);
    static bool validateFaceIntersectionsStrict(PolyhedralBoundedSolid* solid, const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext, java::String* msg);

    /**
    Checks that no two distinct vertices of `solid` are geometrically
    coincident (distance under `context.bigEpsilon()`).
    @param solid solid to inspect
    @param context tolerance context
    @param msg collects one line per coincident pair found
    @return true when no coincident vertices are found
    */
    static bool validateNoCoincidentVertices(PolyhedralBoundedSolid* solid, const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& context, java::String* msg);

    /**
    Checks that face and vertex ids are unique and covered by the stored
    maxima of the solid.
    @param solid solid to inspect
    @param msg collects one line per violation found
    @return true when ids are consistent
    */
    static bool validateUniqueFaceAndVertexIds(PolyhedralBoundedSolid* solid, java::String* msg);
};

#endif

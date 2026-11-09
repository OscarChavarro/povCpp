#ifndef ___POLYHEDRAL_BOUNDED_SOLID_FALLBACK_GEOMETRY__
#define ___POLYHEDRAL_BOUNDED_SOLID_FALLBACK_GEOMETRY__

#include "java/util/Collections.h"
#include "java/util/ArrayList.h"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/_PolyhedralBoundedSolidOperator.h"

class PolyhedralBoundedSolid;

/**
Low-level geometric primitives shared by the structural-shape boolean fallback
builders (profile-difference, axis-aligned cell, orthogonal-profile and
offset-cylinder families). Derives from `_PolyhedralBoundedSolidOperator`
solely to use the shared `numericContext` tolerance state.

C++ counterpart of Java's `_PolyhedralBoundedSolidFallbackGeometry`. Java
null profiles are empty vectors here.
*/
class _PolyhedralBoundedSolidFallbackGeometry : public _PolyhedralBoundedSolidOperator {
public:
    typedef java::ArrayList<Vector3Dd> Profile;

    static double coordinate(const Vector3Dd& p, int axis);
    static bool sameCoordinate(double a, double b);

    /**
    @param a first bounds (minX, minY, minZ, maxX, maxY, maxZ)
    @param b second bounds
    @return true when all six values match within tolerance
    */
    static bool boundsMatch(const double* a, const double* b);

    /**
    Inserts `value` when no coordinate within tolerance is present, keeping
    the list sorted.
    */
    static void addUniqueCoordinate(java::ArrayList<double>& values, double value);
    static java::ArrayList<double> uniqueVertexCoordinates(PolyhedralBoundedSolid* solid, int axis);
    static double signedAreaOnYZ(const Profile& profile);

    /**
    @return the largest loop lying on the plane `x`, or empty (Java null)
    */
    static Profile extractProfileAtX(PolyhedralBoundedSolid* solid, double x);
    static bool sameProfilePoint(const Vector3Dd& a, const Vector3Dd& b);
    static void appendProfilePoint(Profile& profile, const Vector3Dd& point);
    static Vector3Dd projectProfilePoint(const Vector3Dd& point, double x,
        double zCut);
    static Vector3Dd intersectProfileSegmentAtZ(const Vector3Dd& a,
        const Vector3Dd& b, double x, double zCut);
    static Profile clipProfileAboveZ(const Profile& profile, double x,
        double zCut);

private:
    _PolyhedralBoundedSolidFallbackGeometry();
};

#endif

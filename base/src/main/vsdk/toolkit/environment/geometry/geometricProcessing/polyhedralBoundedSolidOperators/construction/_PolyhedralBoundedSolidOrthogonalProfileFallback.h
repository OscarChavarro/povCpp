#ifndef ___POLYHEDRAL_BOUNDED_SOLID_ORTHOGONAL_PROFILE_FALLBACK__
#define ___POLYHEDRAL_BOUNDED_SOLID_ORTHOGONAL_PROFILE_FALLBACK__

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
Structural-shape boolean fallback for orthogonal extruded profiles
(X-extruded YZ and Y-extruded XZ): detects the profile-cell boolean case and
rebuilds the result cell-by-cell.

C++ counterpart of Java's `_PolyhedralBoundedSolidOrthogonalProfileFallback`.
*/
class _PolyhedralBoundedSolidOrthogonalProfileFallback
    : public _PolyhedralBoundedSolidOperator {
public:
    /**
    @return a new solid owned by the caller, or null when the case does not
    apply
    */
    static PolyhedralBoundedSolid* buildOrthogonalProfileBooleanFallback(
        PolyhedralBoundedSolid* inSolidA,
        PolyhedralBoundedSolid* inSolidB,
        int op);

private:
    static const int PROFILE_X_EXTRUDED_YZ = 0;
    static const int PROFILE_Y_EXTRUDED_XZ = 1;

    class OrthogonalProfileOperandSpec {
    public:
        int type;
        double bounds[6];
        java::ArrayList<Vector3Dd> yzProfile;
        java::ArrayList<double> rightBoundaryZ;
        java::ArrayList<double> rightBoundaryX;

        bool contains(double x, double y, double z) const;
        double rightXAtZ(double z) const;
    };

    class OrthogonalProfileBooleanFallbackSpec {
    public:
        OrthogonalProfileOperandSpec operandA;
        OrthogonalProfileOperandSpec operandB;
        OrthogonalProfileOperandSpec yExtruded;
        double xMin;
        double xMax;
        java::ArrayList<double> yCoordinates;
        java::ArrayList<double> zCoordinates;

        double xAtBoundary(int boundary, double z) const;
        Vector3Dd point(int boundary, int iy, int iz) const;
    };

    class ProfileCellBooleanBuilder {
    public:
        ProfileCellBooleanBuilder();
        void addQuad(const Vector3Dd corners[4]);
        PolyhedralBoundedSolid* result();
    private:
        PolyhedralBoundedSolid* solid;
        java::TreeMap<java::String, _PolyhedralBoundedSolidVertex*> vertices;
        java::TreeMap<java::String, _PolyhedralBoundedSolidEdge*> edges;
        int nextVertexId;
        int nextFaceId;

        static long long coordinateKey(double value);
        static java::String vertexKey(const Vector3Dd& point);
        _PolyhedralBoundedSolidVertex* vertexAt(const Vector3Dd& point);
        static java::String edgeKey(_PolyhedralBoundedSolidVertex* a,
            _PolyhedralBoundedSolidVertex* b);
        void attachEdge(_PolyhedralBoundedSolidHalfEdge* he,
            _PolyhedralBoundedSolidVertex* a,
            _PolyhedralBoundedSolidVertex* b);
        static bool degenerateQuad(const Vector3Dd corners[4]);
    };

    _PolyhedralBoundedSolidOrthogonalProfileFallback();
    static bool pointInsideYZProfile(const java::ArrayList<Vector3Dd>& profile,
        double y, double z);
    static bool createXExtrudedYZSpec(PolyhedralBoundedSolid* solid,
        const java::ArrayList<double>& xCoordinates,
        const java::ArrayList<double>& yCoordinates,
        const java::ArrayList<double>& zCoordinates,
        OrthogonalProfileOperandSpec& outSpec);
    static bool createYExtrudedXZSpec(PolyhedralBoundedSolid* solid,
        const java::ArrayList<double>& xCoordinates,
        const java::ArrayList<double>& yCoordinates,
        const java::ArrayList<double>& zCoordinates,
        OrthogonalProfileOperandSpec& outSpec);
    static bool createOrthogonalProfileSpec(PolyhedralBoundedSolid* solid,
        OrthogonalProfileOperandSpec& outSpec);
    static bool prepareOrthogonalProfileBooleanFallbackSpec(
        PolyhedralBoundedSolid* inSolidA,
        PolyhedralBoundedSolid* inSolidB,
        OrthogonalProfileBooleanFallbackSpec& outSpec);
    static bool profileCellSelected(const OrthogonalProfileBooleanFallbackSpec& spec,
        int op, int zone, int iy, int iz);
    static void addProfileBoundaryQuad(ProfileCellBooleanBuilder& builder,
        const OrthogonalProfileBooleanFallbackSpec& spec,
        int zone, int iy, int iz, int side);
};

#endif

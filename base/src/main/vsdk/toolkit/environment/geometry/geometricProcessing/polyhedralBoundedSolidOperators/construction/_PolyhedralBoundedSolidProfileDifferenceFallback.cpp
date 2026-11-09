
#include "java/util/ArrayList.txx"
#include "java/util/Collections.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/PolyhedralBoundedSolidModeler.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/construction/_PolyhedralBoundedSolidProfileDifferenceFallback.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fallbacks/_PolyhedralBoundedSolidFallbackGeometry.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidTopologyEditing.h"

typedef _PolyhedralBoundedSolidProfileDifferenceFallback ProfileFallback;
typedef _PolyhedralBoundedSolidFallbackGeometry FallbackGeometry;

bool ProfileFallback::isBetween(double value, double min, double max)
{
    return value > min + numericContext.bigEpsilon() &&
           value < max - numericContext.bigEpsilon();
}

_PolyhedralBoundedSolidProfileDifferenceFallbackSpec*
ProfileFallback::prepareProfileDifferenceFallbackSpec(
    PolyhedralBoundedSolid* minuend,
    PolyhedralBoundedSolid* subtrahend,
    int op)
{
    // Covers profile-subtraction cases where connect emits only
    // degenerate vertex/face rings and finish returns the full minuend.
    if ( op != SUBTRACT ||
         minuend->getVerticesList().size() <= 0 ||
         subtrahend->getVerticesList().size() <= 0 ) {
        return 0;
    }

    double* minuendBounds = minuend->getMinMax();
    double* subtrahendBounds = subtrahend->getMinMax();
    bool sameBounds = FallbackGeometry::boundsMatch(minuendBounds, subtrahendBounds);
    delete[] subtrahendBounds;
    _PolyhedralBoundedSolidProfileDifferenceFallbackSpec* spec = 0;

    if ( sameBounds ) {
        java::ArrayList<double> minuendX =
            FallbackGeometry::uniqueVertexCoordinates(minuend, 0);
        java::ArrayList<double> subtrahendX =
            FallbackGeometry::uniqueVertexCoordinates(subtrahend, 0);
        java::ArrayList<double> subtrahendZ =
            FallbackGeometry::uniqueVertexCoordinates(subtrahend, 2);
        if ( minuendX.size() == 2 &&
             subtrahendX.size() == 3 &&
             subtrahendZ.size() == 3 ) {
            double xCut = subtrahendX[1];
            double zCut = subtrahendZ[1];
            if ( isBetween(xCut, minuendBounds[0], minuendBounds[3]) &&
                 isBetween(zCut, minuendBounds[2], minuendBounds[5]) ) {
                FallbackGeometry::Profile profile =
                    FallbackGeometry::extractProfileAtX(minuend, minuendBounds[0]);
                FallbackGeometry::Profile clippedProfile =
                    FallbackGeometry::clipProfileAboveZ(profile, xCut, zCut);
                if ( clippedProfile.size() >= 3 ) {
                    java::Collections::reverse(clippedProfile);
                    spec = new _PolyhedralBoundedSolidProfileDifferenceFallbackSpec(
                        clippedProfile, xCut, minuendBounds[3], minuendBounds);
                }
            }
        }
    }
    delete[] minuendBounds;
    return spec;
}

PolyhedralBoundedSolid* ProfileFallback::buildProfileDifferenceFallback(
    _PolyhedralBoundedSolidProfileDifferenceFallbackSpec* spec)
{
    PolyhedralBoundedSolid* solid;
    Matrix4x4d translation;
    int i;

    if ( spec == 0 ||
         spec->clippedProfileAtCut.size() < 3 ||
         spec->xMax <= spec->xCut + numericContext.bigEpsilon() ) {
        return 0;
    }

    int n = (int)spec->clippedProfileAtCut.size();
    solid = new PolyhedralBoundedSolid();
    PolyhedralBoundedSolidEulerOperators::mvfs(solid,
        spec->clippedProfileAtCut[0], 1, 1);
    for ( i = 1; i < n; i++ ) {
        PolyhedralBoundedSolidEulerOperators::smev(solid, 1, i, i + 1,
            spec->clippedProfileAtCut[i]);
    }
    PolyhedralBoundedSolidEulerOperators::mef(solid, 1, 1, n, n - 1, 1, 2, 2);

    translation = translation.translation(spec->xMax - spec->xCut, 0, 0);
    PolyhedralBoundedSolidModeler::translationalSweepExtrudeFacePlanar(
        solid, solid->findFace(1), translation);
    PolyhedralBoundedSolidTopologyEditing::compactIds(solid);
    return solid;
}

PolyhedralBoundedSolid* ProfileFallback::applyProfileDifferenceFallbackIfNeeded(
    _PolyhedralBoundedSolidProfileDifferenceFallbackSpec* spec,
    PolyhedralBoundedSolid* result)
{
    PolyhedralBoundedSolid* fallback;

    if ( spec == 0 || result == 0 ) {
        return result;
    }
    double* resultBounds = result->getMinMax();
    bool sameBounds = FallbackGeometry::boundsMatch(resultBounds, spec->minuendBounds);
    delete[] resultBounds;
    if ( !sameBounds ) {
        return result;
    }

    fallback = buildProfileDifferenceFallback(spec);
    if ( fallback == 0 ) {
        return result;
    }
    return fallback;
}

#include "java/lang/String.h"
#include <cmath>
#include <cstdio>

#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/PolyhedralBoundedSolidModeler.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fixtures/CsgKurlanderBowlFixture.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidValidationEngine.h"

typedef PolyhedralBoundedSolidModeler Modeler;

namespace {

const int CYLINDER_SIDES = 30;
const int MOTIF_RING_COUNT = 4;
const int MOTIFS_PER_TYPE_RING = 5;
const int STAR_COUNT = MOTIF_RING_COUNT * MOTIFS_PER_TYPE_RING;
const int MOON_COUNT = MOTIF_RING_COUNT * MOTIFS_PER_TYPE_RING;
const double STAR_Z_VALUES[] = { 9.0, 6.5, 14.0, 4.0, 11.5 };
const double STAR_AZIMUTH_OFFSETS[] = { 0.0, -22.5, -45.0, -45.0, -67.5 };
const double MOON_Z_VALUES[] = { 4.0, 14.0, 11.5, 9.0, 6.5 };
const double MOON_AZIMUTH_OFFSETS[] = { 0.0, 0.0, -22.5, -45.0, -67.5 };
const double OBJECT_SCALE = 0.1;
const double MOTIF_RADIAL_DISTANCE = 6.0;
const double STAR_AXIS_ROLL_DEGREES = -90.0;
const double MOON_AXIS_ROLL_DEGREES = 90.0;
const double MOON_BOWL_INSET_FRACTION = 0.10;
const double MOON_CYLINDER_HEIGHT = 5.5;
const double MOON_AXIS_PROXIMITY_FRACTION = 0.1;
const double MOON_AXIS_PROXIMITY_REFERENCE_HEIGHT = 5.0;
const double MOON_AXIS_PROXIMITY_SHIFT =
    MOON_AXIS_PROXIMITY_FRACTION *
    (MOON_AXIS_PROXIMITY_REFERENCE_HEIGHT * OBJECT_SCALE);

double toRadians(double degrees)
{
    return java::Math::toRadians(degrees);
}

}

double CsgKurlanderBowlFixture::scale(double value)
{
    return value * OBJECT_SCALE;
}

/**
C++ ownership: the operands are released after the operation.
*/
PolyhedralBoundedSolid* CsgKurlanderBowlFixture::booleanOp(
    PolyhedralBoundedSolid* a, PolyhedralBoundedSolid* b, int op)
{
    PolyhedralBoundedSolid* result = Modeler::setOp(a, b, op, false);
    delete a;
    delete b;
    return result;
}

PolyhedralBoundedSolid* CsgKurlanderBowlFixture::booleanOpWithoutFaceMaximization(
    PolyhedralBoundedSolid* a, PolyhedralBoundedSolid* b, int op)
{
    PolyhedralBoundedSolid* result = Modeler::setOp(a, b, op, false, false);
    delete a;
    delete b;
    return result;
}

PolyhedralBoundedSolid* CsgKurlanderBowlFixture::createSphere(double radius,
    const Vector3Dd& center)
{
    Sphere sphere(radius);
    PolyhedralBoundedSolid* solid = sphere.exportToPolyhedralBoundedSolid();
    Matrix4x4d t;
    t = t.translation(center);
    Modeler::applyTransformation(solid, t);
    return solid;
}

PolyhedralBoundedSolid* CsgKurlanderBowlFixture::createCylinder(double radius,
    double height, const Vector3Dd& translation)
{
    PolyhedralBoundedSolid* solid = Modeler::createCircularLamina(
        0.0, 0.0, radius, 0.0, CYLINDER_SIDES);
    Matrix4x4d sweep;
    sweep = sweep.translation(0.0, 0.0, height);
    Modeler::translationalSweepExtrudeFacePlanar(solid, solid->findFace(1), sweep);

    Matrix4x4d move;
    move = move.translation(translation);
    Modeler::applyTransformation(solid, move);
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);
    return solid;
}

PolyhedralBoundedSolid* CsgKurlanderBowlFixture::createExtrudedPolygon(
    const java::ArrayList<Vector3Dd>& points, double thickness)
{
    int i;
    int n = (int)points.size();
    PolyhedralBoundedSolid* solid = new PolyhedralBoundedSolid();
    PolyhedralBoundedSolidEulerOperators::mvfs(solid, points[0], 1, 1);

    for ( i = 1; i < n; i++ ) {
        PolyhedralBoundedSolidEulerOperators::smev(solid, 1, i, i + 1, points[i]);
    }
    PolyhedralBoundedSolidEulerOperators::smef(solid, 1, n, 1, 2);

    Matrix4x4d t;
    t = t.translation(0.0, 0.0, thickness);
    Modeler::translationalSweepExtrudeFacePlanar(solid, solid->findFace(1), t);
    return solid;
}

PolyhedralBoundedSolid* CsgKurlanderBowlFixture::createStar()
{
    int i;
    int n = 10;
    double outerR = scale(2.0);
    double innerR = scale(0.77);
    double start = toRadians(-90.0);
    java::ArrayList<Vector3Dd> points;

    for ( i = 0; i < n; i++ ) {
        double a = start + i * M_PI / 5.0;
        double r = (i % 2 == 0) ? outerR : innerR;
        points.add(Vector3Dd(r * std::cos(a), r * std::sin(a), 0.0));
    }

    return createExtrudedPolygon(points, scale(5.5));
}

PolyhedralBoundedSolid* CsgKurlanderBowlFixture::createMoon()
{
    PolyhedralBoundedSolid* a = createCylinder(
        scale(1.5), scale(MOON_CYLINDER_HEIGHT), Vector3Dd(0, 0, 0));
    PolyhedralBoundedSolid* b = createCylinder(
        scale(1.5), scale(MOON_CYLINDER_HEIGHT),
        Vector3Dd(scale(1.1), 0, scale(0.6)));
    return booleanOp(a, b, Modeler::SUBTRACT);
}

PolyhedralBoundedSolid* CsgKurlanderBowlFixture::placeStar(
    PolyhedralBoundedSolid* star, double z, double azimuthDeg)
{
    return placeMotif(star, z, azimuthDeg, 1.0, 0.0, STAR_AXIS_ROLL_DEGREES);
}

Matrix4x4d CsgKurlanderBowlFixture::createStarPlacementTransformation(
    double z, double azimuthDeg)
{
    return createMotifPlacementTransformation(z, azimuthDeg, 1.0, 0.0,
        STAR_AXIS_ROLL_DEGREES);
}

PolyhedralBoundedSolid* CsgKurlanderBowlFixture::placeMoon(
    PolyhedralBoundedSolid* moon, double z, double azimuthDeg)
{
    return placeMotif(moon, z, azimuthDeg,
        1.0 - MOON_BOWL_INSET_FRACTION, -MOON_AXIS_PROXIMITY_SHIFT,
        MOON_AXIS_ROLL_DEGREES);
}

Matrix4x4d CsgKurlanderBowlFixture::createMoonPlacementTransformation(
    double z, double azimuthDeg)
{
    return createMotifPlacementTransformation(z, azimuthDeg,
        1.0 - MOON_BOWL_INSET_FRACTION, -MOON_AXIS_PROXIMITY_SHIFT,
        MOON_AXIS_ROLL_DEGREES);
}

PolyhedralBoundedSolid* CsgKurlanderBowlFixture::placeMotif(
    PolyhedralBoundedSolid* motif, double z, double azimuthDeg,
    double radialDistanceFactor, double radialOffset, double axisRollDeg)
{
    Matrix4x4d m = createMotifPlacementTransformation(
        z, azimuthDeg, radialDistanceFactor, radialOffset, axisRollDeg);

    Modeler::applyTransformation(motif, m);
    return motif;
}

/**
Builds the rigid transformation that places a motif on the bowl's inner
surface.
@param z motif height along the bowl axis, in unscaled model units
@param azimuthDeg motif azimuth angle around the bowl axis, in degrees
@param radialDistanceFactor fraction of MOTIF_RADIAL_DISTANCE used as the
motif's nominal radial distance from the bowl axis
@param radialOffset additional radial displacement, in scaled world units;
negative values move the motif closer to the bowl axis
@param axisRollDeg roll angle, in degrees, around the motif placement axis
@return the combined translation/rotation matrix for the motif
*/
Matrix4x4d CsgKurlanderBowlFixture::createMotifPlacementTransformation(
    double z, double azimuthDeg, double radialDistanceFactor,
    double radialOffset, double axisRollDeg)
{
    Matrix4x4d ry;
    Matrix4x4d rz;
    Matrix4x4d roll;
    Matrix4x4d t;
    Matrix4x4d m;
    double azimuthRad = toRadians(azimuthDeg);
    double radialDistance = MOTIF_RADIAL_DISTANCE * radialDistanceFactor;
    double x = scale(radialDistance * std::cos(azimuthRad))
        + radialOffset * std::cos(azimuthRad);
    double y = scale(radialDistance * std::sin(azimuthRad))
        + radialOffset * std::sin(azimuthRad);

    ry = ry.axisRotation(toRadians(90.0), 0, 1, 0);
    rz = rz.axisRotation(toRadians(azimuthDeg), 0, 0, 1);
    roll = roll.axisRotation(toRadians(axisRollDeg), 0, 0, 1);
    t = t.translation(x, y, scale(z));
    m = t.multiply(rz.multiply(ry.multiply(roll)));
    return m;
}

int CsgKurlanderBowlFixture::getSingleMotifStarCount()
{
    return STAR_COUNT;
}

int CsgKurlanderBowlFixture::getSingleMotifMoonCount()
{
    return MOON_COUNT;
}

int CsgKurlanderBowlFixture::getSingleMotifCount()
{
    return STAR_COUNT + MOON_COUNT;
}

int CsgKurlanderBowlFixture::normalizeSingleMotifIndex(int motifIndex)
{
    // As Java `Math.floorMod`
    int n = getSingleMotifCount();
    int r = motifIndex % n;
    return r < 0 ? r + n : r;
}

java::String CsgKurlanderBowlFixture::describeSingleMotif(int motifIndex)
{
    int normalizedIndex = normalizeSingleMotifIndex(motifIndex);
    int lastIndex = getSingleMotifCount() - 1;
    int typeIndex;
    java::String text;

    if ( normalizedIndex < STAR_COUNT ) {
        typeIndex = normalizedIndex + 1;
        text = "STAR " + java::String::valueOf(typeIndex) + "/" +
            java::String::valueOf(STAR_COUNT) + " index " +
            java::String::valueOf(normalizedIndex) + "/" + java::String::valueOf(lastIndex);
        return text.c_str();
    }

    typeIndex = normalizedIndex - STAR_COUNT + 1;
    text = "MOON " + java::String::valueOf(typeIndex) + "/" +
        java::String::valueOf(MOON_COUNT) + " index " +
        java::String::valueOf(normalizedIndex) + "/" + java::String::valueOf(lastIndex);
    return text.c_str();
}

PolyhedralBoundedSolid* CsgKurlanderBowlFixture::createBowl()
{
    PolyhedralBoundedSolid* outer = createSphere(
        scale(10.0), Vector3Dd(0, 0, scale(10.0)));
    PolyhedralBoundedSolid* inner = createSphere(
        scale(9.5), Vector3Dd(0, 0, scale(10.0)));
    PolyhedralBoundedSolid* shell = booleanOp(outer, inner, Modeler::SUBTRACT);
    return booleanOp(
        shell,
        createCylinder(scale(10.5), scale(16.5), Vector3Dd(0, 0, 0)),
        Modeler::INTERSECTION);
}

PolyhedralBoundedSolid* CsgKurlanderBowlFixture::createAllMotifsUnion()
{
    int i;
    int moonCount = getSingleMotifMoonCount();
    int starCount = getSingleMotifStarCount();
    int motifCount = moonCount + starCount;

    printProgressMessage(("createAllMotifsUnion: building " +
        java::String::valueOf(motifCount) + " motifs").c_str());

    PolyhedralBoundedSolid* result =
        placeMoon(createMoon(), getMoonZ(0), getMoonAzimuthDeg(0));
    printMotifProgress("moon", 1, moonCount, 1, motifCount);

    for ( i = 1; i < moonCount; i++ ) {
        printMotifProgress("moon", i + 1, moonCount, i + 1, motifCount);
        result = booleanOpWithoutFaceMaximization(result,
            placeMoon(createMoon(), getMoonZ(i), getMoonAzimuthDeg(i)),
            Modeler::UNION);
    }

    for ( i = 0; i < starCount; i++ ) {
        printMotifProgress("star", i + 1, starCount,
            moonCount + i + 1, motifCount);
        result = booleanOpWithoutFaceMaximization(result,
            placeStar(createStar(), getStarZ(i), getStarAzimuthDeg(i)),
            Modeler::UNION);
    }

    PolyhedralBoundedSolidValidationEngine::validateIntermediate(result);
    printProgressMessage("createAllMotifsUnion: finished");
    return result;
}

java::ArrayList<PolyhedralBoundedSolid*> CsgKurlanderBowlFixture::createBowlAndFirstStarOperands()
{
    return createBowlAndFirstStarOperands(0);
}

java::ArrayList<PolyhedralBoundedSolid*> CsgKurlanderBowlFixture::createBowlAndFirstStarOperands(
    int motifIndex)
{
    java::ArrayList<PolyhedralBoundedSolid*> operands;
    PolyhedralBoundedSolid* outer = createSphere(
        scale(10.0), Vector3Dd(0, 0, scale(10.0)));
    PolyhedralBoundedSolid* inner = createSphere(
        scale(9.5), Vector3Dd(0, 0, scale(10.0)));
    PolyhedralBoundedSolid* shell = booleanOp(outer, inner, Modeler::SUBTRACT);
    PolyhedralBoundedSolid* bowl = booleanOp(
        shell,
        createCylinder(scale(10.5), scale(16.5), Vector3Dd(0, 0, 0)),
        Modeler::INTERSECTION);

    operands.add(bowl);
    operands.add(createSingleMotif(motifIndex));
    return operands;
}

PolyhedralBoundedSolid* CsgKurlanderBowlFixture::createSingleMotif(int motifIndex)
{
    int normalizedIndex = normalizeSingleMotifIndex(motifIndex);
    int motifTypeIndex;

    if ( normalizedIndex < STAR_COUNT ) {
        motifTypeIndex = normalizedIndex;
        return placeStar(createStar(),
            getStarZ(motifTypeIndex),
            getStarAzimuthDeg(motifTypeIndex));
    }

    motifTypeIndex = normalizedIndex - STAR_COUNT;
    return placeMoon(createMoon(),
        getMoonZ(motifTypeIndex),
        getMoonAzimuthDeg(motifTypeIndex));
}

double CsgKurlanderBowlFixture::getStarZ(int motifTypeIndex)
{
    return getMotifValue(motifTypeIndex, STAR_Z_VALUES);
}

double CsgKurlanderBowlFixture::getMoonZ(int motifTypeIndex)
{
    return getMotifValue(motifTypeIndex, MOON_Z_VALUES);
}

double CsgKurlanderBowlFixture::getStarAzimuthDeg(int motifTypeIndex)
{
    return getMotifAzimuthDeg(motifTypeIndex, STAR_AZIMUTH_OFFSETS);
}

double CsgKurlanderBowlFixture::getMoonAzimuthDeg(int motifTypeIndex)
{
    return getMotifAzimuthDeg(motifTypeIndex, MOON_AZIMUTH_OFFSETS);
}

double CsgKurlanderBowlFixture::getMotifValue(int motifTypeIndex,
    const double* values)
{
    int positionIndex = motifTypeIndex % MOTIFS_PER_TYPE_RING;
    return values[positionIndex];
}

double CsgKurlanderBowlFixture::getMotifAzimuthDeg(int motifTypeIndex,
    const double* offsets)
{
    int positionIndex = motifTypeIndex % MOTIFS_PER_TYPE_RING;
    int ringIndex = motifTypeIndex / MOTIFS_PER_TYPE_RING + 1;
    double base = -90.0 * ringIndex;

    return base + offsets[positionIndex];
}

java::ArrayList<PolyhedralBoundedSolid*> CsgKurlanderBowlFixture::createShellAndFirstMoonOperands()
{
    java::ArrayList<PolyhedralBoundedSolid*> operands;
    PolyhedralBoundedSolid* outer = createSphere(
        scale(10.0), Vector3Dd(0, 0, scale(10.0)));
    PolyhedralBoundedSolid* inner = createSphere(
        scale(9.5), Vector3Dd(0, 0, scale(10.0)));

    operands.add(booleanOp(outer, inner, Modeler::SUBTRACT));
    operands.add(placeMoon(createMoon(), 4.0, -90.0));
    return operands;
}

PolyhedralBoundedSolid* CsgKurlanderBowlFixture::create()
{
    int i;
    int moonIndex = 0;
    int starIndex = 0;
    int motifIndex = 0;
    int moonCount = getSingleMotifMoonCount();
    int starCount = getSingleMotifStarCount();
    int motifCount = moonCount + starCount;
    printProgressMessage(
        "Processing Kurlander bowl all motifs: starting base shell");
    PolyhedralBoundedSolid* outer = createSphere(
        scale(10.0), Vector3Dd(0, 0, scale(10.0)));
    PolyhedralBoundedSolid* inner = createSphere(
        scale(9.5), Vector3Dd(0, 0, scale(10.0)));
    PolyhedralBoundedSolid* shell = booleanOp(outer, inner, Modeler::SUBTRACT);
    printProgressMessage(
        "Processing Kurlander bowl all motifs: base shell ready");

    for ( i = 0; i < moonCount; i++ ) {
        moonIndex++;
        motifIndex++;
        printMotifProgress("moon", moonIndex, moonCount, motifIndex, motifCount);
        shell = booleanOpWithoutFaceMaximization(shell,
            placeMoon(createMoon(), getMoonZ(i), getMoonAzimuthDeg(i)),
            Modeler::SUBTRACT);
    }

    for ( i = 0; i < starCount; i++ ) {
        starIndex++;
        motifIndex++;
        printMotifProgress("star", starIndex, starCount, motifIndex, motifCount);
        shell = booleanOpWithoutFaceMaximization(shell,
            placeStar(createStar(), getStarZ(i), getStarAzimuthDeg(i)),
            Modeler::SUBTRACT);
    }

    PolyhedralBoundedSolid* guide = createCylinder(
        scale(10.5), scale(16.5), Vector3Dd(0, 0, 0));
    printProgressMessage(
        "Processing Kurlander bowl all motifs: clipping final bowl");
    PolyhedralBoundedSolid* result = booleanOpWithoutFaceMaximization(
        shell, guide, Modeler::INTERSECTION);

    PolyhedralBoundedSolidValidationEngine::validateIntermediate(result);
    printProgressMessage("Processing Kurlander bowl all motifs: finished");
    return result;
}

void CsgKurlanderBowlFixture::printMotifProgress(const char* motifType,
    int typeIndex, int typeCount, int motifIndex, int motifCount)
{
    printProgressMessage(("Processing " + java::String(motifType) + " " +
        java::String::valueOf(typeIndex) + "/" + java::String::valueOf(typeCount) +
        ", motif " + java::String::valueOf(motifIndex) + "/" +
        java::String::valueOf(motifCount)).c_str());
}

void CsgKurlanderBowlFixture::printProgressMessage(const java::String& message)
{
    printf("%s\n", message.c_str());
    fflush(stdout);
    fprintf(stderr, "%s\n", message.c_str());
    fflush(stderr);
}

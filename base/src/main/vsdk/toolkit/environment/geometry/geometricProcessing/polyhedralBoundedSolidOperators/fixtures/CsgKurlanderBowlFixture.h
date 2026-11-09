#ifndef __CSG_KURLANDER_BOWL_FIXTURE__
#define __CSG_KURLANDER_BOWL_FIXTURE__

#include "java/util/ArrayList.h"

#include "java/lang/String.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class PolyhedralBoundedSolid;

/**
Builds the Kurlander bowl CSG model (a spherical shell with 20 moon and 20
star motifs subtracted, clipped by a guide cylinder) and its partial
operands, used as a stress fixture for the boolean set operations.

C++ counterpart of Java's `CsgKurlanderBowlFixture`. Every returned solid is
new and owned by the caller.
*/
class CsgKurlanderBowlFixture {
public:
    static int getSingleMotifStarCount();
    static int getSingleMotifMoonCount();
    static int getSingleMotifCount();
    static int normalizeSingleMotifIndex(int motifIndex);
    static java::String describeSingleMotif(int motifIndex);

    /**
    @return the bowl base: outer sphere minus inner sphere, clipped by the
    guide cylinder (operand A for all boolean subtraction tests)
    */
    static PolyhedralBoundedSolid* createBowl();

    /**
    @return the union of all 40 motifs (20 moons + 20 stars) as a single
    multi-shell solid
    */
    static PolyhedralBoundedSolid* createAllMotifsUnion();

    static java::ArrayList<PolyhedralBoundedSolid*> createBowlAndFirstStarOperands();
    static java::ArrayList<PolyhedralBoundedSolid*> createBowlAndFirstStarOperands(
        int motifIndex);
    static PolyhedralBoundedSolid* createSingleMotif(int motifIndex);
    static java::ArrayList<PolyhedralBoundedSolid*> createShellAndFirstMoonOperands();

    /**
    @return the complete Kurlander bowl
    */
    static PolyhedralBoundedSolid* create();

    static Matrix4x4d createStarPlacementTransformation(double z,
        double azimuthDeg);
    static Matrix4x4d createMoonPlacementTransformation(double z,
        double azimuthDeg);

private:
    CsgKurlanderBowlFixture();

    static double scale(double value);
    static PolyhedralBoundedSolid* booleanOp(PolyhedralBoundedSolid* a,
        PolyhedralBoundedSolid* b, int op);
    static PolyhedralBoundedSolid* booleanOpWithoutFaceMaximization(
        PolyhedralBoundedSolid* a, PolyhedralBoundedSolid* b, int op);
    static PolyhedralBoundedSolid* createSphere(double radius,
        const Vector3Dd& center);
    static PolyhedralBoundedSolid* createCylinder(double radius, double height,
        const Vector3Dd& translation);
    static PolyhedralBoundedSolid* createExtrudedPolygon(
        const java::ArrayList<Vector3Dd>& points, double thickness);
    static PolyhedralBoundedSolid* createStar();
    static PolyhedralBoundedSolid* createMoon();
    static PolyhedralBoundedSolid* placeStar(PolyhedralBoundedSolid* star,
        double z, double azimuthDeg);
    static PolyhedralBoundedSolid* placeMoon(PolyhedralBoundedSolid* moon,
        double z, double azimuthDeg);
    static PolyhedralBoundedSolid* placeMotif(PolyhedralBoundedSolid* motif,
        double z, double azimuthDeg, double radialDistanceFactor,
        double radialOffset, double axisRollDeg);
    static Matrix4x4d createMotifPlacementTransformation(double z,
        double azimuthDeg, double radialDistanceFactor, double radialOffset,
        double axisRollDeg);
    static double getStarZ(int motifTypeIndex);
    static double getMoonZ(int motifTypeIndex);
    static double getStarAzimuthDeg(int motifTypeIndex);
    static double getMoonAzimuthDeg(int motifTypeIndex);
    static double getMotifValue(int motifTypeIndex, const double* values);
    static double getMotifAzimuthDeg(int motifTypeIndex, const double* offsets);
    static void printMotifProgress(const char* motifType, int typeIndex,
        int typeCount, int motifIndex, int motifCount);
    static void printProgressMessage(const java::String& message);
};

#endif

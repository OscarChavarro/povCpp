#include "java/lang/String.h"
#include <cmath>
#include <ostream>
#include "java/lang/StringBuilder.h"
#include <gtest/gtest.h>
#include "java/lang/Math.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/PolyhedralBoundedSolidModeler.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fixtures/SimpleTestGeometryLibrary.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/Cone.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidTopologySummary.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidValidationEngine.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"

/*
Systematic boolean regression matrix based on reference object pairs and
single-object reference solids. C++ counterpart of Java's
`BooleansFromReferenceObjectPairsTest`: the expected summaries are the Java
literals, converted mechanically, so both ports must produce the same
topology and bounds.
*/
namespace {

typedef PolyhedralBoundedSolidModeler Modeler;

struct TopologicalSummary {
    int shellCount;
    int faceCount;
    int edgeCount;
    int vertexCount;
    int loopCount;
    int multiLoopFaceCount;
    int eulerCharacteristic;
    java::ArrayList<int> shellFaceCountsSorted;
    java::ArrayList<int> loopsPerFaceSorted;
    java::ArrayList<int> verticesPerLoopSorted;
    java::ArrayList<long long> minMaxMicrounits;

    bool operator==(const TopologicalSummary& o) const
    {
        return shellCount == o.shellCount && faceCount == o.faceCount &&
            edgeCount == o.edgeCount && vertexCount == o.vertexCount &&
            loopCount == o.loopCount &&
            multiLoopFaceCount == o.multiLoopFaceCount &&
            eulerCharacteristic == o.eulerCharacteristic &&
            shellFaceCountsSorted.equals(o.shellFaceCountsSorted) &&
            loopsPerFaceSorted.equals(o.loopsPerFaceSorted) &&
            verticesPerLoopSorted.equals(o.verticesPerLoopSorted) &&
            minMaxMicrounits.equals(o.minMaxMicrounits);
    }
};

template <typename T, typename... Items>
java::ArrayList<T> listOf(Items... items)
{
    T values[] = { static_cast<T>(items)... };
    java::ArrayList<T> result;
    for ( const T& value : values ) {
        result.add(value);
    }
    return result;
}

template <typename T>
java::String join(const java::ArrayList<T>& values)
{
    java::StringBuilder out;
    out.append("[");
    for ( int i = 0; i < values.size(); i++ ) {
        out.append(i > 0 ? ", " : "").append(values[i]);
    }
    out.append("]");
    return out.toString();
}

std::ostream& operator<<(std::ostream& out, const TopologicalSummary& s)
{
    return out << "TopologicalSummary{shellCount=" << s.shellCount
        << ", faceCount=" << s.faceCount << ", edgeCount=" << s.edgeCount
        << ", vertexCount=" << s.vertexCount << ", loopCount=" << s.loopCount
        << ", multiLoopFaceCount=" << s.multiLoopFaceCount
        << ", eulerCharacteristic=" << s.eulerCharacteristic
        << ", shellFaceCountsSorted=" << join(s.shellFaceCountsSorted).c_str()
        << ", loopsPerFaceSorted=" << join(s.loopsPerFaceSorted).c_str()
        << ", verticesPerLoopSorted=" << join(s.verticesPerLoopSorted).c_str()
        << ", minMaxMicrounits=" << join(s.minMaxMicrounits).c_str() << "}";
}

TopologicalSummary of(int shellCount, int faceCount, int edgeCount,
    int vertexCount, int loopCount, int multiLoopFaceCount,
    int eulerCharacteristic, java::ArrayList<int> shellFaceCountsSorted,
    java::ArrayList<int> loopsPerFaceSorted, java::ArrayList<int> verticesPerLoopSorted,
    java::ArrayList<long long> minMaxMicrounits)
{
    TopologicalSummary s;
    s.shellCount = shellCount;
    s.faceCount = faceCount;
    s.edgeCount = edgeCount;
    s.vertexCount = vertexCount;
    s.loopCount = loopCount;
    s.multiLoopFaceCount = multiLoopFaceCount;
    s.eulerCharacteristic = eulerCharacteristic;
    s.shellFaceCountsSorted = shellFaceCountsSorted;
    s.loopsPerFaceSorted = loopsPerFaceSorted;
    s.verticesPerLoopSorted = verticesPerLoopSorted;
    s.minMaxMicrounits = minMaxMicrounits;
    return s;
}

TopologicalSummary summaryFrom(PolyhedralBoundedSolid* solid)
{
    TopologicalSummary s;
    s.faceCount = (int)solid->getPolygonsList().size();
    s.edgeCount = (int)solid->getEdgesList().size();
    s.vertexCount = (int)solid->getVerticesList().size();
    s.eulerCharacteristic = s.vertexCount - s.edgeCount + s.faceCount;
    s.loopCount = 0;
    s.multiLoopFaceCount = 0;
    for ( long i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        int loopsInFace = (int)face->boundariesList.size();
        s.loopsPerFaceSorted.add(loopsInFace);
        s.loopCount += loopsInFace;
        if ( loopsInFace > 1 ) {
            s.multiLoopFaceCount++;
        }
        for ( int j = 0; j < loopsInFace; j++ ) {
            s.verticesPerLoopSorted.add(
                (int)face->boundariesList.get(j)->halfEdgesList.size());
        }
    }
    s.loopsPerFaceSorted.sort(nullptr);
    s.verticesPerLoopSorted.sort(nullptr);

    PolyhedralBoundedSolidTopologySummary shared =
        PolyhedralBoundedSolidTopologySummary::from(solid);
    for ( size_t i = 0; i < shared.getShells().size(); i++ ) {
        s.shellFaceCountsSorted.add(shared.getShells()[i].getFaceCount());
    }
    s.shellFaceCountsSorted.sort(nullptr);
    s.shellCount = (int)s.shellFaceCountsSorted.size();

    double* minMax = solid->getMinMax();
    for ( int i = 0; i < 6; i++ ) {
        // As Java `Math.round`
        s.minMaxMicrounits.add((long long)std::floor(minMax[i] * 1000000.0 + 0.5));
    }
    delete[] minMax;
    return s;
}

enum ReferenceBooleanOperation {
    UNION,
    INTERSECTION,
    DIFFERENCE_A_MINUS_B,
    DIFFERENCE_B_MINUS_A
};

const char* operationLabel(ReferenceBooleanOperation op)
{
    switch ( op ) {
      case UNION: return "UNION";
      case INTERSECTION: return "INTERSECTION";
      case DIFFERENCE_A_MINUS_B: return "A-B";
      default: return "B-A";
    }
}

PolyhedralBoundedSolid* createTranslatedBox(double sx, double sy, double sz,
    double tx, double ty, double tz)
{
    Box box(Vector3Dd(sx, sy, sz));
    PolyhedralBoundedSolid* solid = box.exportToPolyhedralBoundedSolid();
    Matrix4x4d translation;
    translation = translation.translation(tx, ty, tz);
    Modeler::applyTransformation(solid, translation);
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);
    return solid;
}

PolyhedralBoundedSolid* createTranslatedCylinder(double radius, double height)
{
    Cone cone(radius, radius, height);
    PolyhedralBoundedSolid* solid = cone.exportToPolyhedralBoundedSolid();
    Matrix4x4d translation;
    translation = translation.translation(0.55, 0.55, 0.05);
    Modeler::applyTransformation(solid, translation);
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);
    return solid;
}

/** Set operation that releases its operands (C++ ownership). */
PolyhedralBoundedSolid* setOpAndRelease(PolyhedralBoundedSolid* a,
    PolyhedralBoundedSolid* b, int op, bool strict)
{
    PolyhedralBoundedSolid* result = Modeler::setOp(a, b, op, false, true, strict);
    delete a;
    delete b;
    return result;
}

/** C++ counterpart of Java's `CsgSampleCorpusFixtures.createPair`. */
java::ArrayList<PolyhedralBoundedSolid*> createPair(const java::String& sample)
{
    java::ArrayList<PolyhedralBoundedSolid*> operands;
    if ( sample == "HOLLOW_BRICK" ) {
        PolyhedralBoundedSolid* a = createTranslatedBox(1.0, 0.2, 0.2, 0.5, 0.1, 0.1);
        PolyhedralBoundedSolid* b = createTranslatedBox(1.0, 0.2, 0.2, 0.5, 0.9, 0.1);
        PolyhedralBoundedSolid* c = createTranslatedBox(0.2, 1.0, 0.2, 0.1, 0.5, 0.1);
        PolyhedralBoundedSolid* d = createTranslatedBox(0.2, 1.0, 0.2, 0.9, 0.5, 0.1);
        operands.add(setOpAndRelease(b, c, Modeler::UNION, true));
        operands.add(setOpAndRelease(a, d, Modeler::UNION, true));
    }
    else if ( sample == "MANT1986_2" ) {
        operands = SimpleTestGeometryLibrary::createTestObjectPairMANT1986_2();
    }
    else if ( sample == "STACKED_BLOCKS" ) {
        operands.add(createTranslatedBox(1.0, 0.5, 0.3, 0.5, 0.5, 0.15));
        operands.add(createTranslatedBox(0.5, 1.0, 0.3, 0.5, 0.5, 0.45));
    }
    else if ( sample == "CROSS_PAIR" ) {
        PolyhedralBoundedSolid* a = createTranslatedBox(1.0, 0.2, 0.2, 0.5, 0.1, 0.1);
        PolyhedralBoundedSolid* c = createTranslatedBox(0.2, 1.0, 0.2, 0.1, 0.5, 0.1);
        PolyhedralBoundedSolid* g = createTranslatedBox(0.2, 0.2, 1.0, 0.1, 0.1, 0.5);
        operands.add(setOpAndRelease(a, c, Modeler::UNION, true));
        operands.add(g);
    }
    else if ( sample == "MOON_BLOCK" ) {
        PolyhedralBoundedSolid* cylinderA = createTranslatedCylinder(0.5, 1.0);
        PolyhedralBoundedSolid* cylinderB = createTranslatedCylinder(0.5, 2.0);
        Matrix4x4d translation;
        translation = translation.translation(0.275, 0.0, -0.5);
        Modeler::applyTransformation(cylinderB, translation);
        PolyhedralBoundedSolidValidationEngine::validateIntermediate(cylinderA);
        PolyhedralBoundedSolidValidationEngine::validateIntermediate(cylinderB);
        operands.add(cylinderA);
        operands.add(cylinderB);
    }
    else if ( sample == "MANT1988_6_13" ) {
        operands = SimpleTestGeometryLibrary::createTestObjectPairMANT1988_6_13();
    }
    else if ( sample == "MANT1988_3" ) {
        operands = SimpleTestGeometryLibrary::createTestObjectPairMANT1988_3();
    }
    else if ( sample == "MANT1988_15_2_HOLED" ) {
        operands = SimpleTestGeometryLibrary::createTestObjectPairMANT1988_15_2(-1);
    }
    else {
        operands = SimpleTestGeometryLibrary::createTestObjectPairMANT1988_15_1();
    }
    return operands;
}

PolyhedralBoundedSolid* runBooleanOperation(ReferenceBooleanOperation operation,
    PolyhedralBoundedSolid* a, PolyhedralBoundedSolid* b)
{
    switch ( operation ) {
      case UNION:
        return Modeler::setOp(a, b, Modeler::UNION, false, true, false);
      case INTERSECTION:
        return Modeler::setOp(a, b, Modeler::INTERSECTION, false, true, false);
      case DIFFERENCE_A_MINUS_B:
        return Modeler::setOp(a, b, Modeler::SUBTRACT, false, true, false);
      case DIFFERENCE_B_MINUS_A:
      default:
        return Modeler::setOp(b, a, Modeler::SUBTRACT, false, true, false);
    }
}

PolyhedralBoundedSolid* createSphere(double radius, int subdivisionsC,
    int subdivisionsH)
{
    Matrix4x4d move;
    move = move.translation(0.55, 0.55, 0.55);
    Sphere sphere(radius);
    PolyhedralBoundedSolid* solid =
        sphere.exportToPolyhedralBoundedSolid(subdivisionsC, subdivisionsH);
    Modeler::applyTransformation(solid, move);
    return solid;
}

PolyhedralBoundedSolid* createCsgLampShellReference()
{
    PolyhedralBoundedSolid* outerSphere = createSphere(0.5, 3, 1);
    PolyhedralBoundedSolid* innerSphere = createSphere(0.45, 3, 1);
    PolyhedralBoundedSolid* sphericalShell = setOpAndRelease(outerSphere,
        innerSphere, Modeler::SUBTRACT, true);

    Box clipCubeGeometry(Vector3Dd(1.4, 1.4, 1.05));
    PolyhedralBoundedSolid* clipCube =
        clipCubeGeometry.exportToPolyhedralBoundedSolid();
    Matrix4x4d cubeMove;
    cubeMove = cubeMove.translation(0.55, 0.55, 0.325);
    Modeler::applyTransformation(clipCube, cubeMove);

    return setOpAndRelease(sphericalShell, clipCube, Modeler::INTERSECTION, true);
}

TopologicalSummary expectedMANT1986_2Union() { return of(1, 12, 30, 20, 12, 0, 2, listOf<int>(12), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4, 6, 6, 6, 6, 6, 6), listOf<long long>(0LL, -180000LL, 0LL, 1240000LL, 500000LL, 1020000LL)); }
TopologicalSummary expectedMANT1986_2Intersection() { return of(1, 6, 12, 8, 6, 0, 2, listOf<int>(6), listOf<int>(1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4), listOf<long long>(240000LL, 0LL, 420000LL, 1000000LL, 320000LL, 600000LL)); }
TopologicalSummary expectedMANT1986_2DifferenceAB() { return of(1, 9, 21, 14, 9, 0, 2, listOf<int>(9), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4, 6, 6, 6), listOf<long long>(0LL, 0LL, 0LL, 1000000LL, 500000LL, 600000LL)); }
TopologicalSummary expectedMANT1986_2DifferenceBA() { return of(1, 9, 21, 14, 9, 0, 2, listOf<int>(9), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4, 6, 6, 6), listOf<long long>(240000LL, -180000LL, 420000LL, 1240000LL, 320000LL, 1020000LL)); }
TopologicalSummary expectedSTACKED_BLOCKSUnion() { return of(1, 14, 32, 20, 14, 0, 2, listOf<int>(14), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 6, 6, 6, 6), listOf<long long>(0LL, 0LL, 0LL, 1000000LL, 1000000LL, 600000LL)); }
TopologicalSummary expectedSTACKED_BLOCKSIntersection() { return of(1, 2, 4, 4, 2, 0, 2, listOf<int>(2), listOf<int>(1, 1), listOf<int>(4, 4), listOf<long long>(250000LL, 250000LL, 300000LL, 750000LL, 750000LL, 300000LL)); }
TopologicalSummary expectedSTACKED_BLOCKSDifferenceAB() { return of(1, 6, 12, 8, 6, 0, 2, listOf<int>(6), listOf<int>(1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4), listOf<long long>(0LL, 250000LL, 0LL, 1000000LL, 750000LL, 300000LL)); }
TopologicalSummary expectedSTACKED_BLOCKSDifferenceBA() { return of(1, 6, 12, 8, 6, 0, 2, listOf<int>(6), listOf<int>(1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4), listOf<long long>(250000LL, 0LL, 300000LL, 750000LL, 1000000LL, 600000LL)); }
TopologicalSummary expectedMOON_BLOCKUnion() { return of(1, 76, 222, 148, 76, 0, 2, listOf<int>(76), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 8, 8, 36, 36, 38, 38), listOf<long long>(50000LL, 50000LL, -450000LL, 1325000LL, 1050000LL, 1550000LL)); }
TopologicalSummary expectedMOON_BLOCKIntersection() { return of(1, 34, 96, 64, 34, 0, 2, listOf<int>(34), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 32, 32), listOf<long long>(325000LL, 71175LL, 50000LL, 1050000LL, 1028825LL, 1050000LL)); }
TopologicalSummary expectedMOON_BLOCKDifferenceAB() { return of(1, 40, 114, 76, 40, 0, 2, listOf<int>(40), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 38, 38), listOf<long long>(50000LL, 50000LL, 50000LL, 687500LL, 1050000LL, 1050000LL)); }
TopologicalSummary expectedMOON_BLOCKDifferenceBA() { return of(1, 70, 204, 136, 70, 0, 2, listOf<int>(70), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 8, 8, 32, 32, 36, 36), listOf<long long>(325000LL, 50000LL, -450000LL, 1325000LL, 1050000LL, 1550000LL)); }
TopologicalSummary expectedCROSS_PAIRUnion() { return of(1, 12, 27, 17, 12, 0, 2, listOf<int>(12), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4, 4, 4, 4, 6, 6, 6), listOf<long long>(0LL, 0LL, 0LL, 1000000LL, 1000000LL, 1000000LL)); }
TopologicalSummary expectedHOLLOW_BRICKUnion() { return of(1, 10, 24, 16, 12, 2, 2, listOf<int>(10), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 2, 2), listOf<int>(4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4), listOf<long long>(0LL, 0LL, 0LL, 1000000LL, 1000000LL, 200000LL)); }
TopologicalSummary expectedHOLLOW_BRICKIntersection() { return of(2, 12, 24, 16, 12, 0, 4, listOf<int>(6, 6), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4), listOf<long long>(0LL, 0LL, 0LL, 1000000LL, 1000000LL, 200000LL)); }
TopologicalSummary expectedHOLLOW_BRICKDifferenceAB() { return of(1, 8, 18, 12, 8, 0, 2, listOf<int>(8), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4, 6, 6), listOf<long long>(0LL, 200000LL, 0LL, 800000LL, 1000000LL, 200000LL)); }
TopologicalSummary expectedHOLLOW_BRICKDifferenceBA() { return of(1, 8, 18, 12, 8, 0, 2, listOf<int>(8), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4, 6, 6), listOf<long long>(200000LL, 0LL, 0LL, 1000000LL, 800000LL, 200000LL)); }
TopologicalSummary expectedMANT1988_6_13Union() { return of(1, 11, 27, 18, 11, 0, 2, listOf<int>(11), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4, 4, 5, 5, 8, 8), listOf<long long>(0LL, 0LL, 0LL, 1000000LL, 1000000LL, 324324LL)); }
TopologicalSummary expectedMANT1988_6_13Intersection() { return of(1, 12, 30, 20, 12, 0, 2, listOf<int>(12), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 8, 8), listOf<long long>(0LL, 0LL, 0LL, 837838LL, 1000000LL, 324324LL)); }
TopologicalSummary expectedMANT1988_6_13DifferenceAB() { return of(1, 7, 15, 10, 7, 0, 2, listOf<int>(7), listOf<int>(1, 1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 5, 5), listOf<long long>(0LL, 243243LL, 81081LL, 837838LL, 756757LL, 324324LL)); }
TopologicalSummary expectedMANT1988_6_13DifferenceBA() { return of(1, 12, 30, 20, 12, 0, 2, listOf<int>(12), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 8, 8), listOf<long long>(432432LL, 0LL, 0LL, 1000000LL, 1000000LL, 324324LL)); }
TopologicalSummary expectedMANT1988_15_2HoledUnion() { return of(1, 14, 30, 20, 16, 2, 4, listOf<int>(14), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2), listOf<int>(3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4), listOf<long long>(0LL, 0LL, 0LL, 775000LL, 1000000LL, 600000LL)); }
TopologicalSummary expectedMANT1988_15_2HoledIntersection() { return of(1, 7, 15, 12, 9, 2, 4, listOf<int>(7), listOf<int>(1, 1, 1, 1, 1, 2, 2), listOf<int>(3, 3, 3, 3, 3, 3, 4, 4, 4), listOf<long long>(137500LL, 225000LL, 250000LL, 637500LL, 775000LL, 550000LL)); }
TopologicalSummary expectedMANT1988_15_2HoledDifferenceAB() { return of(1, 9, 21, 14, 11, 2, 2, listOf<int>(9), listOf<int>(1, 1, 1, 1, 1, 1, 1, 2, 2), listOf<int>(3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4), listOf<long long>(137500LL, 0LL, 0LL, 637500LL, 1000000LL, 600000LL)); }
TopologicalSummary expectedMANT1988_15_2HoledDifferenceBA() { return of(2, 12, 24, 18, 14, 2, 6, listOf<int>(6, 6), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2), listOf<int>(3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4), listOf<long long>(0LL, 225000LL, 250000LL, 775000LL, 775000LL, 550000LL)); }
TopologicalSummary expectedMANT1988_15_1Union() { return of(1, 10, 24, 16, 10, 0, 2, listOf<int>(10), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(3, 3, 4, 4, 4, 4, 6, 6, 6, 8), listOf<long long>(0LL, 0LL, 0LL, 1000000LL, 1000000LL, 1000000LL)); }
TopologicalSummary expectedMANT1988_15_1Intersection() { return of(1, 10, 24, 16, 10, 0, 2, listOf<int>(10), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4, 6, 6, 6, 6), listOf<long long>(0LL, 0LL, 0LL, 1000000LL, 1000000LL, 1000000LL)); }
TopologicalSummary expectedMANT1988_15_1DifferenceAB() { return of(2, 10, 18, 12, 10, 0, 4, listOf<int>(5, 5), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(3, 3, 3, 3, 4, 4, 4, 4, 4, 4), listOf<long long>(0LL, 0LL, 583333LL, 333333LL, 1000000LL, 1000000LL)); }
TopologicalSummary expectedMANT1988_15_1DifferenceBA() { return of(1, 8, 18, 12, 8, 0, 2, listOf<int>(8), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4, 6, 6), listOf<long long>(333333LL, 0LL, 250000LL, 1000000LL, 1000000LL, 1000000LL)); }
TopologicalSummary expectedMANT1988_3Union() { return of(1, 14, 35, 23, 14, 0, 2, listOf<int>(14), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4, 4, 5, 5, 6, 6, 6, 6, 8), listOf<long long>(50000LL, -160000LL, 50000LL, 970000LL, 470000LL, 770000LL)); }
TopologicalSummary expectedMANT1988_3Intersection() { return of(1, 6, 12, 8, 6, 0, 2, listOf<int>(6), listOf<int>(1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4), listOf<long long>(750000LL, 50000LL, 230000LL, 970000LL, 260000LL, 410000LL)); }
TopologicalSummary expectedMANT1988_3DifferenceAB() { return of(1, 11, 27, 18, 11, 0, 2, listOf<int>(11), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4, 4, 6, 6, 6, 8), listOf<long long>(50000LL, 50000LL, 50000LL, 970000LL, 470000LL, 770000LL)); }
TopologicalSummary expectedMANT1988_3DifferenceBA() { return of(1, 8, 18, 12, 8, 0, 2, listOf<int>(8), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1), listOf<int>(4, 4, 4, 4, 4, 4, 6, 6), listOf<long long>(390000LL, -160000LL, 230000LL, 970000LL, 260000LL, 410000LL)); }
TopologicalSummary expectedLampShellSummary() { return of(1, 13, 24, 14, 14, 1, 3, listOf<int>(13), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2), listOf<int>(3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4), listOf<long long>(300000LL, 116987LL, 50000LL, 1050000LL, 983013LL, 850000LL)); }
TopologicalSummary expectedFeaturedObjectSummary() { return of(2, 32, 84, 54, 34, 2, 2, listOf<int>(16, 16), listOf<int>(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2), listOf<int>(4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 6, 6, 8, 8, 8, 8, 10, 10), listOf<long long>(0LL, 0LL, 0LL, 1000000LL, 1000000LL, 1000000LL)); }

/**
C++ counterpart of the Java test's local `KurlanderBowlBuilder`.
*/
class KurlanderBowlBuilder {
public:
    static PolyhedralBoundedSolid* create()
    {
        PolyhedralBoundedSolid* outer = createSphere(s(10.0), Vector3Dd(0, 0, s(10.0)));
        PolyhedralBoundedSolid* inner = createSphere(s(9.5), Vector3Dd(0, 0, s(10.0)));
        PolyhedralBoundedSolid* shell = booleanOp(outer, inner, Modeler::SUBTRACT);

        int i;
        for ( i = 1; i <= 4; i++ ) {
            double base = -90.0 * i;
            shell = booleanOp(shell, placeMotif(createMoon(), 4.0, base), Modeler::SUBTRACT);
            shell = booleanOp(shell, placeMotif(createMoon(), 14.0, base), Modeler::SUBTRACT);
            shell = booleanOp(shell, placeMotif(createMoon(), 11.5, base - 22.5), Modeler::SUBTRACT);
            shell = booleanOp(shell, placeMotif(createMoon(), 9.0, base - 45.0), Modeler::SUBTRACT);
            shell = booleanOp(shell, placeMotif(createMoon(), 6.5, base - 67.5), Modeler::SUBTRACT);
        }

        for ( i = 1; i <= 4; i++ ) {
            double base = -90.0 * i;
            shell = booleanOp(shell, placeMotif(createStar(), 9.0, base), Modeler::SUBTRACT);
            shell = booleanOp(shell, placeMotif(createStar(), 6.5, base - 22.5), Modeler::SUBTRACT);
            shell = booleanOp(shell, placeMotif(createStar(), 14.0, base - 45.0), Modeler::SUBTRACT);
            shell = booleanOp(shell, placeMotif(createStar(), 4.0, base - 45.0), Modeler::SUBTRACT);
            shell = booleanOp(shell, placeMotif(createStar(), 11.5, base - 67.5), Modeler::SUBTRACT);
        }

        PolyhedralBoundedSolid* guide = createCylinder(s(10.5), s(16.5), Vector3Dd(0, 0, 0));
        return booleanOp(shell, guide, Modeler::INTERSECTION);
    }

private:
    static const int CYLINDER_SIDES = 30;

    static double s(double value)
    {
        return value * 0.1;
    }

    static PolyhedralBoundedSolid* booleanOp(PolyhedralBoundedSolid* a,
        PolyhedralBoundedSolid* b, int op)
    {
        return setOpAndRelease(a, b, op, true);
    }

    static PolyhedralBoundedSolid* createSphere(double radius, const Vector3Dd& center)
    {
        Sphere sphere(radius);
        PolyhedralBoundedSolid* solid = sphere.exportToPolyhedralBoundedSolid();
        Matrix4x4d t;
        t = t.translation(center);
        Modeler::applyTransformation(solid, t);
        return solid;
    }

    static PolyhedralBoundedSolid* createCylinder(double radius, double height,
        const Vector3Dd& translation)
    {
        PolyhedralBoundedSolid* solid = Modeler::createCircularLamina(
            0.0, 0.0, radius, 0.0, CYLINDER_SIDES);
        Matrix4x4d sweep;
        sweep = sweep.translation(0.0, 0.0, height);
        Modeler::translationalSweepExtrudeFacePlanar(solid, solid->findFace(1), sweep);
        Matrix4x4d move;
        move = move.translation(translation);
        Modeler::applyTransformation(solid, move);
        return solid;
    }

    static PolyhedralBoundedSolid* createExtrudedPolygon(
        const java::ArrayList<Vector3Dd>& points, double thickness)
    {
        int n = (int)points.size();
        PolyhedralBoundedSolid* solid = new PolyhedralBoundedSolid();
        PolyhedralBoundedSolidEulerOperators::mvfs(solid, points[0], 1, 1);
        for ( int i = 1; i < n; i++ ) {
            PolyhedralBoundedSolidEulerOperators::smev(solid, 1, i, i + 1, points[i]);
        }
        PolyhedralBoundedSolidEulerOperators::smef(solid, 1, n, 1, 2);
        Matrix4x4d t;
        t = t.translation(0.0, 0.0, thickness);
        Modeler::translationalSweepExtrudeFacePlanar(solid, solid->findFace(1), t);
        return solid;
    }

    static PolyhedralBoundedSolid* createStar()
    {
        int n = 10;
        double outerR = s(2.0);
        double innerR = s(0.77);
        double start = java::Math::toRadians(-90.0);
        java::ArrayList<Vector3Dd> points;
        for ( int i = 0; i < n; i++ ) {
            double a = start + i * M_PI / 5.0;
            double r = (i % 2 == 0) ? outerR : innerR;
            points.add(Vector3Dd(r * std::cos(a), r * std::sin(a), 0.0));
        }
        return createExtrudedPolygon(points, s(5.5));
    }

    static PolyhedralBoundedSolid* createMoon()
    {
        PolyhedralBoundedSolid* a = createCylinder(s(1.5), s(5.0), Vector3Dd(0, 0, 0));
        PolyhedralBoundedSolid* b = createCylinder(s(1.5), s(5.0), Vector3Dd(s(1.1), 0, s(0.6)));
        return booleanOp(a, b, Modeler::SUBTRACT);
    }

    static PolyhedralBoundedSolid* placeMotif(PolyhedralBoundedSolid* motif,
        double z, double azimuthDeg)
    {
        Matrix4x4d t;
        Matrix4x4d ry;
        Matrix4x4d rz;
        t = t.translation(s(6.0), 0.0, s(z));
        ry = ry.axisRotation(java::Math::toRadians(90.0), 0, 1, 0);
        rz = rz.axisRotation(java::Math::toRadians(azimuthDeg), 0, 0, 1);
        Matrix4x4d m = rz.multiply(ry.multiply(t));
        Modeler::applyTransformation(motif, m);
        return motif;
    }
};

struct ReferencePairCase {
    const char* sample;
    ReferenceBooleanOperation operation;
    TopologicalSummary (*expected)();
};

const ReferencePairCase REFERENCE_PAIRS[] = {
    { "MANT1986_2", UNION, expectedMANT1986_2Union },
    { "MANT1986_2", INTERSECTION, expectedMANT1986_2Intersection },
    { "MANT1986_2", DIFFERENCE_A_MINUS_B, expectedMANT1986_2DifferenceAB },
    { "MANT1986_2", DIFFERENCE_B_MINUS_A, expectedMANT1986_2DifferenceBA },
    { "STACKED_BLOCKS", UNION, expectedSTACKED_BLOCKSUnion },
    { "STACKED_BLOCKS", INTERSECTION, expectedSTACKED_BLOCKSIntersection },
    { "STACKED_BLOCKS", DIFFERENCE_A_MINUS_B, expectedSTACKED_BLOCKSDifferenceAB },
    { "STACKED_BLOCKS", DIFFERENCE_B_MINUS_A, expectedSTACKED_BLOCKSDifferenceBA },
    { "MOON_BLOCK", UNION, expectedMOON_BLOCKUnion },
    { "MOON_BLOCK", INTERSECTION, expectedMOON_BLOCKIntersection },
    { "MOON_BLOCK", DIFFERENCE_A_MINUS_B, expectedMOON_BLOCKDifferenceAB },
    { "MOON_BLOCK", DIFFERENCE_B_MINUS_A, expectedMOON_BLOCKDifferenceBA },
    { "CROSS_PAIR", UNION, expectedCROSS_PAIRUnion },
    { "HOLLOW_BRICK", UNION, expectedHOLLOW_BRICKUnion },
    { "HOLLOW_BRICK", INTERSECTION, expectedHOLLOW_BRICKIntersection },
    { "HOLLOW_BRICK", DIFFERENCE_A_MINUS_B, expectedHOLLOW_BRICKDifferenceAB },
    { "HOLLOW_BRICK", DIFFERENCE_B_MINUS_A, expectedHOLLOW_BRICKDifferenceBA },
    { "MANT1988_6_13", UNION, expectedMANT1988_6_13Union },
    { "MANT1988_6_13", INTERSECTION, expectedMANT1988_6_13Intersection },
    { "MANT1988_6_13", DIFFERENCE_A_MINUS_B, expectedMANT1988_6_13DifferenceAB },
    { "MANT1988_6_13", DIFFERENCE_B_MINUS_A, expectedMANT1988_6_13DifferenceBA },
    { "MANT1988_15_2_HOLED", UNION, expectedMANT1988_15_2HoledUnion },
    { "MANT1988_15_2_HOLED", INTERSECTION, expectedMANT1988_15_2HoledIntersection },
    { "MANT1988_15_2_HOLED", DIFFERENCE_A_MINUS_B, expectedMANT1988_15_2HoledDifferenceAB },
    { "MANT1988_15_2_HOLED", DIFFERENCE_B_MINUS_A, expectedMANT1988_15_2HoledDifferenceBA },
    { "MANT1988_15_1", UNION, expectedMANT1988_15_1Union },
    { "MANT1988_15_1", INTERSECTION, expectedMANT1988_15_1Intersection },
    { "MANT1988_15_1", DIFFERENCE_A_MINUS_B, expectedMANT1988_15_1DifferenceAB },
    { "MANT1988_15_1", DIFFERENCE_B_MINUS_A, expectedMANT1988_15_1DifferenceBA },
    { "MANT1988_3", UNION, expectedMANT1988_3Union },
    { "MANT1988_3", INTERSECTION, expectedMANT1988_3Intersection },
    { "MANT1988_3", DIFFERENCE_A_MINUS_B, expectedMANT1988_3DifferenceAB },
    { "MANT1988_3", DIFFERENCE_B_MINUS_A, expectedMANT1988_3DifferenceBA },
};

}

TEST(BooleansFromReferenceObjectPairsTest, ReferencePairsMatchJavaTopologySummaries) {
    for ( size_t i = 0; i < sizeof(REFERENCE_PAIRS)/sizeof(REFERENCE_PAIRS[0]); i++ ) {
        const ReferencePairCase& c = REFERENCE_PAIRS[i];
        SCOPED_TRACE((java::String(c.sample) + " + " + operationLabel(c.operation)).c_str());

        java::ArrayList<PolyhedralBoundedSolid*> operands = createPair(c.sample);
        PolyhedralBoundedSolid* result =
            runBooleanOperation(c.operation, operands[0], operands[1]);

        EXPECT_TRUE(PolyhedralBoundedSolidValidationEngine::validateIntermediate(result));
        EXPECT_EQ(c.expected(), summaryFrom(result));
        delete result;
        delete operands[0];
        delete operands[1];
    }
}

TEST(BooleansFromReferenceObjectPairsTest, CsgLampShellMatchesJavaTopologySummary) {
    PolyhedralBoundedSolid* result = createCsgLampShellReference();
    EXPECT_TRUE(PolyhedralBoundedSolidValidationEngine::validateIntermediate(result));
    EXPECT_EQ(expectedLampShellSummary(), summaryFrom(result));
    delete result;
}

TEST(BooleansFromReferenceObjectPairsTest, FeaturedObjectMatchesJavaTopologySummary) {
    PolyhedralBoundedSolid* result = SimpleTestGeometryLibrary::createTestObjectAPPE1967_3();
    EXPECT_TRUE(PolyhedralBoundedSolidValidationEngine::validateIntermediate(result));
    EXPECT_EQ(expectedFeaturedObjectSummary(), summaryFrom(result));
    delete result;
}

// As Java: `CSG_KURLANDER_BOWL` is marked as failing in the current legacy
// matrix and remains pending explicit revalidation (no hardcoded reference
// summary yet)
TEST(BooleansFromReferenceObjectPairsTest, DISABLED_CsgKurlanderBowlMatchesReferenceTopologySummary) {
    PolyhedralBoundedSolid* result = KurlanderBowlBuilder::create();
    EXPECT_TRUE(PolyhedralBoundedSolidValidationEngine::validateIntermediate(result));
    delete result;
    FAIL() << "Missing hardcoded reference summary: CSG_KURLANDER_BOWL";
}

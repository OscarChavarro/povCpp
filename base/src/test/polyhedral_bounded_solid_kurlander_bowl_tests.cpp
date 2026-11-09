#include "java/lang/Double.h"
#include "java/lang/RuntimeException.h"
#include "java/lang/String.h"
#include <cmath>
#include <gtest/gtest.h>
#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/PolyhedralBoundedSolidModeler.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/topology/_PolyhedralBoundedSolidSetFinisher.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/topology/_PolyhedralBoundedSolidSetNullEdgesConnector.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fixtures/CsgKurlanderBowlFixture.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidNumericPolicy.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidValidationEngine.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

/*
C++ counterparts of Java's `CsgKurlanderBowlAllMotifsRegressionTest` and
`CsgKurlanderBowlFirstStarRegressionTest`. C++ ownership: the operands of a
set operation stay owned by the test, which releases them after the
operation.
*/
namespace {

typedef PolyhedralBoundedSolidModeler Modeler;
typedef PolyhedralBoundedSolidNumericPolicy::ToleranceContext ToleranceContext;

const double GEOMETRY_TOLERANCE = 1.0e-9;
const double TOP_MOUTH_TOLERANCE = 1.0e-7;
const int FIRST_MOON_MOTIF_INDEX = 20;
const int THIRD_MOON_MOTIF_INDEX = 22;
const int STAR_VERTEX_COUNT = 10;

/** Runs `setOp` and releases the operands. */
PolyhedralBoundedSolid* setOpAndRelease(java::ArrayList<PolyhedralBoundedSolid*>& operands,
    int op, bool maximizeResultFaces)
{
    PolyhedralBoundedSolid* result = Modeler::setOp(operands[0], operands[1],
        op, false, maximizeResultFaces);
    delete operands[0];
    delete operands[1];
    operands.clear();
    return result;
}

double maxZ(PolyhedralBoundedSolid* solid)
{
    double* minMax = solid->getMinMax();
    double value = minMax[5];
    delete[] minMax;
    return value;
}

void expectVectorClose(const Vector3Dd& actual, const Vector3Dd& expected)
{
    EXPECT_NEAR(expected.x(), actual.x(), GEOMETRY_TOLERANCE);
    EXPECT_NEAR(expected.y(), actual.y(), GEOMETRY_TOLERANCE);
    EXPECT_NEAR(expected.z(), actual.z(), GEOMETRY_TOLERANCE);
}

bool loopStaysOnZ(_PolyhedralBoundedSolidLoop* loop, double z)
{
    _PolyhedralBoundedSolidHalfEdge* halfEdge = loop->boundaryStartHalfEdge;
    _PolyhedralBoundedSolidHalfEdge* start = halfEdge;

    do {
        if ( std::fabs(halfEdge->startingVertex->position.z() - z) >
             TOP_MOUTH_TOLERANCE ) {
            return false;
        }
        halfEdge = halfEdge->next();
    } while ( halfEdge != start );
    return true;
}

bool isTopFace(_PolyhedralBoundedSolidFace* face, double topZ)
{
    if ( face == 0 || face->boundariesList.size() < 1 ) {
        return false;
    }
    for ( long i = 0; i < face->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(i);
        if ( loop == 0 || loop->boundaryStartHalfEdge == 0 ||
             !loopStaysOnZ(loop, topZ) ) {
            return false;
        }
    }
    return true;
}

double findExtremeRadius(_PolyhedralBoundedSolidFace* face, bool minimum)
{
    double radius = minimum ? java::Double::POSITIVE_INFINITY :
        -java::Double::POSITIVE_INFINITY;

    for ( long i = 0; i < face->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(i);
        _PolyhedralBoundedSolidHalfEdge* halfEdge = loop->boundaryStartHalfEdge;
        _PolyhedralBoundedSolidHalfEdge* start = halfEdge;

        do {
            double currentRadius = std::hypot(
                halfEdge->startingVertex->position.x(),
                halfEdge->startingVertex->position.y());
            radius = minimum ? std::fmin(radius, currentRadius) :
                std::fmax(radius, currentRadius);
            halfEdge = halfEdge->next();
        } while ( halfEdge != start );
    }
    return radius;
}

int countHalfEdges(_PolyhedralBoundedSolidFace* face)
{
    int count = 0;
    for ( long i = 0; i < face->boundariesList.size(); i++ ) {
        _PolyhedralBoundedSolidLoop* loop = face->boundariesList.get(i);
        if ( loop != 0 ) {
            count += (int)loop->halfEdgesList.size();
        }
    }
    return count;
}

double findReferenceTopInnerMouthRadius(PolyhedralBoundedSolid* referenceBowl,
    double topZ)
{
    double innerRadius = java::Double::POSITIVE_INFINITY;

    for ( long i = 0; i < referenceBowl->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = referenceBowl->getPolygonsList().get(i);
        if ( isTopFace(face, topZ) && face->boundariesList.size() > 1 ) {
            innerRadius = std::fmin(innerRadius, findExtremeRadius(face, true));
        }
    }
    if ( std::isinf(innerRadius) ) {
        throw java::RuntimeException(
            "Expected reference bowl to expose an open top annulus");
    }
    return innerRadius;
}

bool isTopMouthCapFace(_PolyhedralBoundedSolidFace* face, double topZ,
    double innerMouthRadius)
{
    return isTopFace(face, topZ) &&
           face->boundariesList.size() == 1 &&
           countHalfEdges(face) >= 3 &&
           findExtremeRadius(face, false) <= innerMouthRadius +
               TOP_MOUTH_TOLERANCE;
}

int countTopMouthCapFaces(PolyhedralBoundedSolid* result, double topZ,
    double innerMouthRadius)
{
    int count = 0;
    for ( long i = 0; i < result->getPolygonsList().size(); i++ ) {
        if ( isTopMouthCapFace(result->getPolygonsList().get(i), topZ,
                 innerMouthRadius) ) {
            count++;
        }
    }
    return count;
}

bool hasSameBounds(const double* actual, const double* expected)
{
    for ( int i = 0; i < 6; i++ ) {
        if ( std::fabs(actual[i] - expected[i]) > GEOMETRY_TOLERANCE ) {
            return false;
        }
    }
    return true;
}

void expectNonEmptyAndIntermediateValid(PolyhedralBoundedSolid* result)
{
    ASSERT_NE((PolyhedralBoundedSolid*)0, result);
    EXPECT_GT(result->getPolygonsList().size(), 0);
    EXPECT_GT(result->getEdgesList().size(), 0);
    EXPECT_GT(result->getVerticesList().size(), 0);
    EXPECT_TRUE(PolyhedralBoundedSolidValidationEngine::validateIntermediate(result));
}

//= First star helpers ==============================================

java::ArrayList<Vector3Dd> createStarPoints()
{
    java::ArrayList<Vector3Dd> points;
    double outerRadius = 2.0;
    double innerRadius = 0.77;
    double start = java::Math::toRadians(-90.0);

    for ( int i = 0; i < STAR_VERTEX_COUNT; i++ ) {
        double angle = start + i * M_PI / 5.0;
        double radius = (i % 2 == 0) ? outerRadius : innerRadius;
        points.add(Vector3Dd(radius * std::cos(angle),
            radius * std::sin(angle), 0.0));
    }
    return points;
}

PolyhedralBoundedSolid* createClosedDoubleWalkStarLamina()
{
    PolyhedralBoundedSolid* solid = new PolyhedralBoundedSolid();
    java::ArrayList<Vector3Dd> starPoints = createStarPoints();
    int vertexId = 1;
    int i;

    PolyhedralBoundedSolidEulerOperators::mvfs(solid, starPoints[0], 1, 1);
    for ( i = 1; i < (int)starPoints.size(); i++ ) {
        vertexId++;
        PolyhedralBoundedSolidEulerOperators::smev(solid, 1, vertexId - 1,
            vertexId, starPoints[i]);
    }
    for ( i = (int)starPoints.size() - 2; i >= 1; i-- ) {
        vertexId++;
        PolyhedralBoundedSolidEulerOperators::smev(solid, 1, vertexId - 1,
            vertexId, starPoints[i]);
    }
    PolyhedralBoundedSolidEulerOperators::smef(solid, 1, vertexId, 1, 2);
    return solid;
}

double loopAreaMagnitude(_PolyhedralBoundedSolidLoop* loop)
{
    _PolyhedralBoundedSolidHalfEdge* he = loop->boundaryStartHalfEdge;
    _PolyhedralBoundedSolidHalfEdge* start = he;
    Vector3Dd normalAccumulator;

    do {
        Vector3Dd p = he->startingVertex->position;
        Vector3Dd q = he->next()->startingVertex->position;
        normalAccumulator = normalAccumulator.add(Vector3Dd(
            (p.y() - q.y()) * (p.z() + q.z()),
            (p.z() - q.z()) * (p.x() + q.x()),
            (p.x() - q.x()) * (p.y() + q.y())));
        he = he->next();
    } while ( he != start );
    return normalAccumulator.length();
}

bool hasPreviousMatchingPosition(_PolyhedralBoundedSolidLoop* loop,
    long currentIndex, _PolyhedralBoundedSolidHalfEdge* current,
    const ToleranceContext& numericContext)
{
    for ( long i = 0; i < currentIndex; i++ ) {
        _PolyhedralBoundedSolidHalfEdge* previous = loop->halfEdgesList.get(i);
        if ( PolyhedralBoundedSolidNumericPolicy::pointsCoincident(
                 previous->startingVertex->position,
                 current->startingVertex->position, numericContext) ) {
            return true;
        }
    }
    return false;
}

int countUniqueLoopPositions(_PolyhedralBoundedSolidLoop* loop,
    const ToleranceContext& numericContext)
{
    int uniqueCount = 0;
    for ( long i = 0; i < loop->halfEdgesList.size(); i++ ) {
        if ( !hasPreviousMatchingPosition(loop, i, loop->halfEdgesList.get(i),
                 numericContext) ) {
            uniqueCount++;
        }
    }
    return uniqueCount;
}

bool isZeroAreaClosedStarDoubleWalkLoop(_PolyhedralBoundedSolidLoop* loop,
    const ToleranceContext& numericContext)
{
    if ( loop == 0 || loop->boundaryStartHalfEdge == 0 ||
         loop->halfEdgesList.size() < (STAR_VERTEX_COUNT - 1) * 2 ) {
        return false;
    }
    if ( loopAreaMagnitude(loop) > numericContext.bigEpsilon() ) {
        return false;
    }
    return countUniqueLoopPositions(loop, numericContext) <= STAR_VERTEX_COUNT;
}

int countZeroAreaClosedStarDoubleWalkLoops(PolyhedralBoundedSolid* solid,
    const ToleranceContext& numericContext)
{
    int count = 0;
    for ( long i = 0; i < solid->getPolygonsList().size(); i++ ) {
        _PolyhedralBoundedSolidFace* face = solid->getPolygonsList().get(i);
        for ( long j = 0; j < face->boundariesList.size(); j++ ) {
            if ( isZeroAreaClosedStarDoubleWalkLoop(face->boundariesList.get(j),
                     numericContext) ) {
                count++;
            }
        }
    }
    return count;
}

}

//= CsgKurlanderBowlAllMotifsRegressionTest ===========================

TEST(CsgKurlanderBowlAllMotifsRegressionTest, BowlMinusFirstMoonStaysNonEmptyAndKeepsMouthOpen) {
    java::ArrayList<PolyhedralBoundedSolid*> operands =
        CsgKurlanderBowlFixture::createBowlAndFirstStarOperands(FIRST_MOON_MOTIF_INDEX);
    double topZ = maxZ(operands[0]);
    double innerMouthRadius = findReferenceTopInnerMouthRadius(operands[0], topZ);
    PolyhedralBoundedSolid* result = setOpAndRelease(operands,
        Modeler::SUBTRACT, true);

    expectNonEmptyAndIntermediateValid(result);
    EXPECT_EQ(0, countTopMouthCapFaces(result, topZ, innerMouthRadius))
        << "Subtracting the moon must keep the bowl mouth open; "
           "no planar cap should be created over the inner top rim";
    delete result;
}

TEST(CsgKurlanderBowlAllMotifsRegressionTest, BowlMinusThirdMoonModifiesTheBowl) {
    java::ArrayList<PolyhedralBoundedSolid*> operands =
        CsgKurlanderBowlFixture::createBowlAndFirstStarOperands(THIRD_MOON_MOTIF_INDEX);
    long originalFaces = operands[0]->getPolygonsList().size();
    long originalEdges = operands[0]->getEdgesList().size();
    long originalVertices = operands[0]->getVerticesList().size();
    double* originalBounds = operands[0]->getMinMax();
    PolyhedralBoundedSolid* result = setOpAndRelease(operands,
        Modeler::SUBTRACT, true);

    expectNonEmptyAndIntermediateValid(result);
    double* resultBounds = result->getMinMax();
    bool sameShape = result->getPolygonsList().size() == originalFaces &&
        result->getEdgesList().size() == originalEdges &&
        result->getVerticesList().size() == originalVertices &&
        hasSameBounds(resultBounds, originalBounds);
    EXPECT_FALSE(sameShape)
        << "Subtracting the third moon must actually modify the bowl; "
           "returning the unmodified operand A hides the failed cut";
    delete[] resultBounds;
    delete[] originalBounds;
    delete result;
}

TEST(CsgKurlanderBowlAllMotifsRegressionTest, ShellMinusFirstMoonStaysValid) {
    java::ArrayList<PolyhedralBoundedSolid*> operands =
        CsgKurlanderBowlFixture::createShellAndFirstMoonOperands();
    PolyhedralBoundedSolid* result = setOpAndRelease(operands,
        Modeler::SUBTRACT, false);

    expectNonEmptyAndIntermediateValid(result);
    delete result;
}

TEST(CsgKurlanderBowlAllMotifsRegressionTest, FirstStarPlacementKeepsTopTipUprightAgainstZ) {
    Matrix4x4d placement =
        CsgKurlanderBowlFixture::createStarPlacementTransformation(9.0, -90.0);
    Vector3Dd origin = placement.multiply(Vector3Dd());
    Vector3Dd extrusionAxis = placement.multiply(
        Vector3Dd(0.0, 0.0, 0.55)).subtract(origin);
    Vector3Dd topTip = placement.multiply(
        Vector3Dd(0.0, -0.2, 0.0)).subtract(origin);

    expectVectorClose(origin, Vector3Dd(0.0, -0.6, 0.9));
    expectVectorClose(extrusionAxis, Vector3Dd(0.0, -0.55, 0.0));
    expectVectorClose(topTip, Vector3Dd(0.0, 0.0, 0.2));
}

TEST(CsgKurlanderBowlAllMotifsRegressionTest, FirstMoonOperandIsRolledAndInsetIntoBowl) {
    java::ArrayList<PolyhedralBoundedSolid*> operands =
        CsgKurlanderBowlFixture::createBowlAndFirstStarOperands(FIRST_MOON_MOTIF_INDEX);
    PolyhedralBoundedSolid* moon = operands[1];
    double* minMax = moon->getMinMax();
    Matrix4x4d placement =
        CsgKurlanderBowlFixture::createMoonPlacementTransformation(4.0, -90.0);
    Vector3Dd origin = placement.multiply(Vector3Dd());
    Vector3Dd cylinderAxis = placement.multiply(
        Vector3Dd(0.0, 0.0, 0.5)).subtract(origin);
    Vector3Dd crescentOffset = placement.multiply(
        Vector3Dd(0.11, 0.0, 0.06)).subtract(origin);

    EXPECT_GT(moon->getVerticesList().size(), 0);
    EXPECT_NEAR(-1.04, minMax[1], GEOMETRY_TOLERANCE);
    EXPECT_NEAR(-0.49, minMax[4], GEOMETRY_TOLERANCE);
    expectVectorClose(origin, Vector3Dd(0.0, -0.49, 0.4));
    expectVectorClose(cylinderAxis, Vector3Dd(0.0, -0.5, 0.0));
    expectVectorClose(crescentOffset, Vector3Dd(0.11, -0.06, 0.0));
    delete[] minMax;
    delete operands[0];
    delete operands[1];
}

//= CsgKurlanderBowlFirstStarRegressionTest ===========================

TEST(CsgKurlanderBowlFirstStarRegressionTest, SingleMotifSelectorPutsStarsBeforeMoonsAndWraps) {
    int starCount = CsgKurlanderBowlFixture::getSingleMotifStarCount();
    int moonCount = CsgKurlanderBowlFixture::getSingleMotifMoonCount();
    int motifCount = CsgKurlanderBowlFixture::getSingleMotifCount();

    EXPECT_EQ(20, starCount);
    EXPECT_EQ(20, moonCount);
    EXPECT_EQ(40, motifCount);
    EXPECT_EQ(0, CsgKurlanderBowlFixture::normalizeSingleMotifIndex(0));
    EXPECT_EQ(0, CsgKurlanderBowlFixture::normalizeSingleMotifIndex(motifCount));
    EXPECT_EQ(motifCount - 1, CsgKurlanderBowlFixture::normalizeSingleMotifIndex(-1));
    EXPECT_EQ(0, java::String(CsgKurlanderBowlFixture::describeSingleMotif(0).c_str())
        .indexOf("STAR 1/20"));
    EXPECT_EQ(0, java::String(CsgKurlanderBowlFixture::describeSingleMotif(starCount).c_str())
        .indexOf("MOON 1/20"));
}

TEST(CsgKurlanderBowlFirstStarRegressionTest, BowlMinusFirstStarStaysNonEmptyAndIntermediateValid) {
    java::ArrayList<PolyhedralBoundedSolid*> operands =
        CsgKurlanderBowlFixture::createBowlAndFirstStarOperands(0);
    PolyhedralBoundedSolid* result = setOpAndRelease(operands,
        Modeler::SUBTRACT, true);

    expectNonEmptyAndIntermediateValid(result);
    delete result;
}

TEST(CsgKurlanderBowlFirstStarRegressionTest, BowlMinusFirstStarConnectClosesAllStarEdges) {
    java::ArrayList<PolyhedralBoundedSolid*> operands =
        CsgKurlanderBowlFixture::createBowlAndFirstStarOperands();
    PolyhedralBoundedSolid* result = setOpAndRelease(operands,
        Modeler::SUBTRACT, true);

    ASSERT_NE((PolyhedralBoundedSolid*)0, result);
    EXPECT_EQ(0, _PolyhedralBoundedSolidSetNullEdgesConnector::getLastLooseACount());
    EXPECT_EQ(0, _PolyhedralBoundedSolidSetNullEdgesConnector::getLastLooseBCount());
    delete result;
}

TEST(CsgKurlanderBowlFirstStarRegressionTest, BowlMinusFirstStarHasNoZeroAreaDoubleWalkLoop) {
    java::ArrayList<PolyhedralBoundedSolid*> operands =
        CsgKurlanderBowlFixture::createBowlAndFirstStarOperands();
    PolyhedralBoundedSolid* result = setOpAndRelease(operands,
        Modeler::SUBTRACT, true);
    ToleranceContext numericContext = PolyhedralBoundedSolidNumericPolicy::forSolid(result);

    EXPECT_EQ(0, countZeroAreaClosedStarDoubleWalkLoops(result, numericContext));
    delete result;
}

TEST(CsgKurlanderBowlFirstStarRegressionTest, BowlMinusFifthStarIsValidAndPairIndexMatchingSucceeds) {
    java::ArrayList<PolyhedralBoundedSolid*> operands =
        CsgKurlanderBowlFixture::createBowlAndFirstStarOperands(4);
    PolyhedralBoundedSolid* result = setOpAndRelease(operands,
        Modeler::SUBTRACT, true);

    ASSERT_NE((PolyhedralBoundedSolid*)0, result);
    EXPECT_GT(result->getPolygonsList().size(), 0);
    EXPECT_GT(result->getEdgesList().size(), 0);
    EXPECT_GT(result->getVerticesList().size(), 0);
    // Section 9.1: pairIndex matching must work for the curved-surface case
    EXPECT_EQ(0, _PolyhedralBoundedSolidSetFinisher::getLastLegacyFallbackCount())
        << "sanitizePairedFaces must not fall back to legacy ordering for star 5";
    EXPECT_TRUE(PolyhedralBoundedSolidValidationEngine::validateIntermediate(result))
        << "result of bowl minus star 5 must pass intermediate validation";
    delete result;
}

TEST(CsgKurlanderBowlFirstStarRegressionTest, ClosedStarLoopWalkedForwardAndBackwardIsDetected) {
    PolyhedralBoundedSolid* solid = createClosedDoubleWalkStarLamina();
    ToleranceContext numericContext = PolyhedralBoundedSolidNumericPolicy::forSolid(solid);

    EXPECT_GT(countZeroAreaClosedStarDoubleWalkLoops(solid, numericContext), 0);
    delete solid;
}

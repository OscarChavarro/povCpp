#include <gtest/gtest.h>
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/PolyhedralBoundedSolidModeler.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/_PolyhedralBoundedSolidOperator.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/_SetOperationContext.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/classification/_PolyhedralBoundedSolidSetClassifier.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/intersection/_PolyhedralBoundedSolidSetIntersector.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/topology/_PolyhedralBoundedSolidSetFinisher.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/topology/_PolyhedralBoundedSolidSetNullEdgesConnector.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fallbacks/_PolyhedralBoundedSolidIdNamespace.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidTopologySummary.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidValidationEngine.h"

/*
Regression coverage for the connect ([MANT1988].15.7) and finish
([MANT1988].15.15) stages, run after generate and classify over box pairs.
Expected counts match the Java port (cross-port oracle of the boolean
pipeline port plan).
*/
namespace {

/** Reaches the protected pipeline state, as the set operator does. */
struct PipelineAccess : _PolyhedralBoundedSolidOperator {
    static void prepare(PolyhedralBoundedSolid* a, PolyhedralBoundedSolid* b,
        _PolyhedralBoundedSolidIdNamespace* ns)
    {
        setIdNamespace(ns);
        PolyhedralBoundedSolidNumericPolicy::ToleranceContext c =
            PolyhedralBoundedSolidNumericPolicy::forSolids(a, b);
        setNumericContext(&c);
        _PolyhedralBoundedSolidSetOperatorNullEdge::setNumericContext(&numericContext);
    }
    static void release()
    {
        setIdNamespace(nullptr);
    }
};

PolyhedralBoundedSolid* createBox(double sx, double sy, double sz,
    double tx, double ty, double tz)
{
    Box box(Vector3Dd(sx, sy, sz));
    PolyhedralBoundedSolid* solid = box.exportToPolyhedralBoundedSolid();
    Matrix4x4d t;
    t = t.translation(tx, ty, tz);
    PolyhedralBoundedSolidModeler::applyTransformation(solid, t);
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);
    return solid;
}

PolyhedralBoundedSolidTopologySummary runPipeline(PolyhedralBoundedSolid* a,
    PolyhedralBoundedSolid* b, int op, bool* outValid)
{
    _PolyhedralBoundedSolidIdNamespace::updmaxnames(b, a);
    _PolyhedralBoundedSolidIdNamespace ns(a, b);
    PipelineAccess::prepare(a, b, &ns);

    _PolyhedralBoundedSolidSetIntersector::GenerationResult generation =
        _PolyhedralBoundedSolidSetIntersector::setOpGenerate(a, b);
    _SetOperationContext ctx;
    ctx.sonvv = generation.sonvv();
    ctx.sonva = generation.sonva();
    ctx.sonvb = generation.sonvb();
    _PolyhedralBoundedSolidSetClassifier::runSetOpClassify(op, a, b, 0, ctx);

    _PolyhedralBoundedSolidSetNullEdgesConnector connector;
    _PolyhedralBoundedSolidSetNullEdgesConnector::ConnectResult connected =
        connector.connect(op, 0, ctx.sonea, ctx.soneb);
    ctx.sonfa = connected.sonfa();
    ctx.sonfb = connected.sonfb();
    EXPECT_EQ(0, _PolyhedralBoundedSolidSetNullEdgesConnector::getLastLooseACount());
    EXPECT_EQ(0, _PolyhedralBoundedSolidSetNullEdgesConnector::getLastLooseBCount());
    EXPECT_TRUE(_PolyhedralBoundedSolidSetNullEdgesConnector::lastCurveReport
        .isCleanlyClosed());

    PolyhedralBoundedSolid* result = new PolyhedralBoundedSolid();
    _PolyhedralBoundedSolidSetFinisher::finish(a, b, result, op, 0,
        ctx.sonfa, ctx.sonfb);
    EXPECT_EQ(0, _PolyhedralBoundedSolidSetFinisher::getLastLegacyFallbackCount());
    PipelineAccess::release();

    *outValid = PolyhedralBoundedSolidValidationEngine::validateIntermediate(result);
    PolyhedralBoundedSolidTopologySummary summary =
        PolyhedralBoundedSolidTopologySummary::from(result);
    // The finish stage shares edges and vertices between the operands and the
    // result (Java garbage collected ownership), so only the result owns its
    // nodes here; the operand leftovers are released with the leak pass of
    // the port plan.
    a->getPolygonsList().clear();
    a->getEdgesList().clear();
    a->getVerticesList().clear();
    b->getPolygonsList().clear();
    b->getEdgesList().clear();
    b->getVerticesList().clear();
    delete result;
    delete a;
    delete b;
    return summary;
}

void expectCounts(const PolyhedralBoundedSolidTopologySummary& s,
    int faces, int edges, int vertices)
{
    EXPECT_EQ(faces, s.getFaceCount());
    EXPECT_EQ(edges, s.getEdgeCount());
    EXPECT_EQ(vertices, s.getVertexCount());
    EXPECT_EQ(1, s.getShellCount());
    EXPECT_FALSE(s.hasUniversalContradiction()) << s.toString().c_str();
}

}

TEST(PolyhedralBoundedSolidSetConnectFinishTest, CrossingBoxesUnion) {
    bool valid = false;
    PolyhedralBoundedSolidTopologySummary s = runPipeline(
        createBox(1,1,1, 0,0,0), createBox(1,1,1, 0.5,0.3,0.2),
        _PolyhedralBoundedSolidOperator::UNION, &valid);
    EXPECT_TRUE(valid);
    expectCounts(s, 12, 30, 20);
}

TEST(PolyhedralBoundedSolidSetConnectFinishTest, CrossingBoxesIntersection) {
    bool valid = false;
    PolyhedralBoundedSolidTopologySummary s = runPipeline(
        createBox(1,1,1, 0,0,0), createBox(1,1,1, 0.5,0.3,0.2),
        _PolyhedralBoundedSolidOperator::INTERSECTION, &valid);
    EXPECT_TRUE(valid);
    expectCounts(s, 6, 12, 8);
}

TEST(PolyhedralBoundedSolidSetConnectFinishTest, CrossingBoxesSubtract) {
    bool valid = false;
    PolyhedralBoundedSolidTopologySummary s = runPipeline(
        createBox(1,1,1, 0,0,0), createBox(1,1,1, 0.5,0.3,0.2),
        _PolyhedralBoundedSolidOperator::SUBTRACT, &valid);
    EXPECT_TRUE(valid);
    expectCounts(s, 9, 21, 14);
}

TEST(PolyhedralBoundedSolidSetConnectFinishTest, ThroughBoxUnionHasTwoCurves) {
    bool valid = false;
    PolyhedralBoundedSolidTopologySummary s = runPipeline(
        createBox(1,1,1, 0,0,0), createBox(0.4,0.4,2, 0.1,0.1,0),
        _PolyhedralBoundedSolidOperator::UNION, &valid);
    EXPECT_TRUE(valid);
    expectCounts(s, 16, 36, 24);
}

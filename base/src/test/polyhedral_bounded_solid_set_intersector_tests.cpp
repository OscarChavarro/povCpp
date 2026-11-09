#include <gtest/gtest.h>
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/PolyhedralBoundedSolidModeler.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/booleans/intersection/_PolyhedralBoundedSolidSetIntersector.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fallbacks/_PolyhedralBoundedSolidIdNamespace.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidValidationEngine.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

/*
Big phase 0 of the set operations ([MANT1988].15.2): coincidences between
two boxes, as given by the Java port for the same input.
*/
namespace {

/** Runs the generation as the set operator prepares it */
struct GenerationFixture : _PolyhedralBoundedSolidOperator {
    static _PolyhedralBoundedSolidSetIntersector::GenerationResult generate(
        PolyhedralBoundedSolid* a, PolyhedralBoundedSolid* b)
    {
        _PolyhedralBoundedSolidIdNamespace::updmaxnames(b, a);
        _PolyhedralBoundedSolidIdNamespace ns(a, b);
        setIdNamespace(&ns);
        PolyhedralBoundedSolidNumericPolicy::ToleranceContext context =
            PolyhedralBoundedSolidNumericPolicy::forSolids(a, b);
        setNumericContext(&context);
        _PolyhedralBoundedSolidSetIntersector::GenerationResult result =
            _PolyhedralBoundedSolidSetIntersector::setOpGenerate(a, b);
        setIdNamespace(nullptr);
        return result;
    }
};

PolyhedralBoundedSolid* createBoxSolid(double size, double tx, double ty,
                                       double tz)
{
    Box box(Vector3Dd(size, size, size));
    PolyhedralBoundedSolid* solid = box.exportToPolyhedralBoundedSolid();
    Matrix4x4d translation;
    translation = translation.translation(tx, ty, tz);
    PolyhedralBoundedSolidModeler::applyTransformation(solid, translation);
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);
    return solid;
}

void expectVertex(_PolyhedralBoundedSolidVertex* v, int id, double x,
                  double y, double z)
{
    EXPECT_EQ(v->id, id);
    EXPECT_NEAR(v->position.x(), x, 1e-12);
    EXPECT_NEAR(v->position.y(), y, 1e-12);
    EXPECT_NEAR(v->position.z(), z, 1e-12);
}

}

TEST(PolyhedralBoundedSolidSetIntersectorTest, CrossingBoxesGiveVertexFaceCoincidences) {
    PolyhedralBoundedSolid* a = createBoxSolid(1, 0, 0, 0);
    PolyhedralBoundedSolid* b = createBoxSolid(1, 0.5, 0.3, 0.2);

    _PolyhedralBoundedSolidSetIntersector::GenerationResult r =
        GenerationFixture::generate(a, b);

    EXPECT_EQ(_PolyhedralBoundedSolidSetIntersector::intersectionTrace.size(), 6u);
    EXPECT_STREQ(_PolyhedralBoundedSolidSetIntersector::intersectionTrace[0].c_str(),
        "Vertex A:17 created after intersection between B:8 face and A:<7, 3> edge.");
    EXPECT_EQ(r.sonvv().size(), 0u);
    ASSERT_EQ(r.sonva().size(), 3u);
    ASSERT_EQ(r.sonvb().size(), 3u);
    expectVertex(r.sonva()[0].v, 17, 0.5, 0.5, -0.3);
    EXPECT_EQ(r.sonva()[0].f->id, 8);
    expectVertex(r.sonvb()[2].v, 22, 0.0, -0.2, 0.5);
    EXPECT_EQ(r.sonvb()[2].f->id, 6);
    delete a;
    delete b;
}

TEST(PolyhedralBoundedSolidSetIntersectorTest, TouchingBoxesGiveVertexVertexCoincidences) {
    PolyhedralBoundedSolid* a = createBoxSolid(1, 0, 0, 0);
    PolyhedralBoundedSolid* b = createBoxSolid(1, 1, 0, 0);

    _PolyhedralBoundedSolidSetIntersector::GenerationResult r =
        GenerationFixture::generate(a, b);

    EXPECT_EQ(_PolyhedralBoundedSolidSetIntersector::intersectionTrace.size(), 0u);
    ASSERT_EQ(r.sonvv().size(), 4u);
    expectVertex(r.sonvv()[0].va, 3, 0.5, 0.5, -0.5);
    expectVertex(r.sonvv()[0].vb, 12, 0.5, 0.5, -0.5);
    EXPECT_EQ(r.sonva().size(), 0u);
    EXPECT_EQ(r.sonvb().size(), 0u);
    delete a;
    delete b;
}

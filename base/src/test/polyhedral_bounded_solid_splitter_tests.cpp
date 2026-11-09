#include <gtest/gtest.h>
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/PolyhedralBoundedSolidModeler.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/polyhedralBoundedSolidOperators/fixtures/SimpleTestGeometryLibrary.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidTopologySummary.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidValidationEngine.h"

/*
Regression coverage for the solid/plane split used by
PolyhedralBoundedSolidExample SPLIT_TEST_PART_2 and SPLIT_TEST_PART_3.
C++ counterpart of Java's `PolyhedralBoundedSolidSplitterMantylaRegressionTest`.
*/
namespace {

void assertValidSolid(PolyhedralBoundedSolid* solid, const char* side)
{
    PolyhedralBoundedSolidTopologySummary topology =
        PolyhedralBoundedSolidTopologySummary::from(solid);

    EXPECT_TRUE(PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid))
        << side << " split result must pass intermediate validation";
    EXPECT_GT(topology.getFaceCount(), 0) << side << " faces";
    EXPECT_GT(topology.getShellCount(), 0) << side << " shells";
    EXPECT_FALSE(topology.hasUniversalContradiction())
        << side << " topology: " << topology.toString().c_str();
}

}

TEST(PolyhedralBoundedSolidSplitterMantylaRegressionTest,
     MantylaFixtureSplitAtZPointThreeGivesTwoValidSolids) {
    PolyhedralBoundedSolid* input =
        SimpleTestGeometryLibrary::createTestObjectMANT1986_1();
    InfinitePlane splittingPlane(Vector3Dd(0, 0, 1), Vector3Dd(0, 0, 0.30));
    java::ArrayList<PolyhedralBoundedSolid*> above;
    java::ArrayList<PolyhedralBoundedSolid*> below;

    EXPECT_NO_THROW(PolyhedralBoundedSolidModeler::split(
        input, splittingPlane, above, below));

    ASSERT_EQ(above.size(), 1);
    ASSERT_EQ(below.size(), 1);
    assertValidSolid(above.get(0), "above");
    assertValidSolid(below.get(0), "below");

    delete above.get(0);
    delete below.get(0);
    delete input;
}

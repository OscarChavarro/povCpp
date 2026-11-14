#include <gtest/gtest.h>
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/AabbBoundingVolume.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/AxisAlignedBoundingBox.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/RaySlabReciprocals.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"

TEST(RaySlabReciprocals, DegenerateComponentsUseTheOriginTest) {
    RaySlabReciprocals r(Vector3Dd(2.0, 5e-13, -4.0));
    EXPECT_DOUBLE_EQ(0.5, r.getInverseX());
    EXPECT_FALSE(r.isDegenerateX());
    EXPECT_TRUE(r.isDegenerateY());
    EXPECT_EQ(0.0, r.getInverseY());
    EXPECT_DOUBLE_EQ(-0.25, r.getInverseZ());
    EXPECT_FALSE(r.isDegenerateZ());
    RaySlabReciprocals edge(Vector3Dd(1e-12, -1e-12, 0.0));
    EXPECT_FALSE(edge.isDegenerateX());
    EXPECT_FALSE(edge.isDegenerateY());
    EXPECT_TRUE(edge.isDegenerateZ());
}

TEST(AxisAlignedBoundingBox, ForwardSlabTestAgreesWithCachedReciprocals) {
    AxisAlignedBoundingBox box(Vector3Dd(-1, -1, -1), Vector3Dd(1, 1, 1));
    const Vector3Dd origins[] = {
        Vector3Dd(-3, 0, 0), Vector3Dd(3, 0, 0), Vector3Dd(0, 0, 0),
        Vector3Dd(-3, 2, 0), Vector3Dd(-3, 1, 0), Vector3Dd(0, 0, 5)};
    const Vector3Dd directions[] = {
        Vector3Dd(1, 0, 0), Vector3Dd(1, 0, 0), Vector3Dd(0, 1, 0),
        Vector3Dd(1, 0, 0), Vector3Dd(1, 0, 0), Vector3Dd(0.1, 0.1, -1)};
    const bool expected[] = {true, false, true, false, true, true};
    for (int i = 0; i < 6; i++) {
        Ray ray(origins[i], directions[i]);
        EXPECT_EQ(expected[i], box.intersectsRayForward(ray)) << i;
        EXPECT_EQ(expected[i],
            box.intersectsRayForward(ray, RaySlabReciprocals(directions[i]))) << i;
    }
}

TEST(AxisAlignedBoundingBox, BoundedSlabTestStopsAtMaximumParameter) {
    AxisAlignedBoundingBox box(Vector3Dd(2, -1, -1), Vector3Dd(4, 1, 1));
    Ray ray(Vector3Dd(0, 0, 0), Vector3Dd(1, 0, 0));
    EXPECT_TRUE(box.intersectsRayBefore(ray, 10.0));
    EXPECT_TRUE(box.intersectsRayBefore(ray, 2.0));
    EXPECT_FALSE(box.intersectsRayBefore(ray, 1.5));
    Ray backwards(Vector3Dd(0, 0, 0), Vector3Dd(-1, 0, 0));
    EXPECT_FALSE(box.intersectsRayBefore(backwards, 10.0));
    EXPECT_FALSE(box.intersectsRayForward(backwards));
}

TEST(AxisAlignedBoundingBox, CombinationsAndUnboundedState) {
    AxisAlignedBoundingBox a(Vector3Dd(0, 0, 0), Vector3Dd(2, 2, 2));
    AxisAlignedBoundingBox b(Vector3Dd(1, -1, 1), Vector3Dd(3, 1, 4));
    AxisAlignedBoundingBox u = a.enclosing(b);
    EXPECT_EQ(Vector3Dd(0, -1, 0), u.getMin());
    EXPECT_EQ(Vector3Dd(3, 2, 4), u.getMax());
    AxisAlignedBoundingBox n = a.intersection(b);
    EXPECT_EQ(Vector3Dd(1, 0, 1), n.getMin());
    EXPECT_EQ(Vector3Dd(2, 1, 2), n.getMax());
    EXPECT_EQ(Vector3Dd(1, 1, 1), a.centroid());
    EXPECT_DOUBLE_EQ(5.0, AxisAlignedBoundingBox::empty().expandedBy(Vector3Dd(5, 6, 7)).getMin().x());
    EXPECT_DOUBLE_EQ(2.0, b.extent(0));
    EXPECT_DOUBLE_EQ(3.0, b.extent(2));
    EXPECT_TRUE(AxisAlignedBoundingBox::unbounded().isUnbounded());
    EXPECT_FALSE(a.isUnbounded());
    EXPECT_TRUE(a.containsPointWithTolerance(Vector3Dd(2.05, 1, 1), 0.1));
    EXPECT_FALSE(a.containsPointWithTolerance(Vector3Dd(2.2, 1, 1), 0.1));
}

TEST(AxisAlignedBoundingBox, TransformedCornersEncloseThePlacedBox) {
    Matrix4x4d translation;
    translation = translation.translation(10, 0, 0);
    AxisAlignedBoundingBox placed = AxisAlignedBoundingBox::fromTransformedCorners(
        Vector3Dd(-1, -1, -1), Vector3Dd(1, 1, 1), &translation);
    AxisAlignedBoundingBox same = AxisAlignedBoundingBox::fromTransformedCorners(
        Vector3Dd(-1, -1, -1), Vector3Dd(1, 1, 1), nullptr);
    EXPECT_EQ(Vector3Dd(-1, -1, -1), same.getMin());
    EXPECT_FALSE(placed.isUnbounded());
    EXPECT_TRUE(placed.containsPointWithTolerance(placed.centroid(), 0.0));
    EXPECT_DOUBLE_EQ(2.0, placed.extent(0));
    EXPECT_DOUBLE_EQ(9.0, placed.getMin().x());
    EXPECT_DOUBLE_EQ(11.0, placed.getMax().x());
}

TEST(AxisAlignedBoundingBox, FromGeometryBoundsKeepsOnlyFiniteBounds) {
    GeometryBounds finite;
    ASSERT_TRUE(finite.setFinite(Vector3Dd(-1, -2, -3), Vector3Dd(4, 5, 6)));
    AxisAlignedBoundingBox box = AxisAlignedBoundingBox::fromGeometryBounds(finite);
    EXPECT_EQ(Vector3Dd(-1, -2, -3), box.getMin());
    EXPECT_EQ(Vector3Dd(4, 5, 6), box.getMax());
    GeometryBounds other;
    other.setUnbounded();
    EXPECT_TRUE(AxisAlignedBoundingBox::fromGeometryBounds(other).isUnbounded());
    other.setEmpty();
    EXPECT_TRUE(AxisAlignedBoundingBox::fromGeometryBounds(other).isUnbounded());
    other.setIndeterminate();
    EXPECT_TRUE(AxisAlignedBoundingBox::fromGeometryBounds(other).isUnbounded());
}

TEST(AabbBoundingVolume, FiltersAreConservativeAndCopiesAreIndependent) {
    AabbBoundingVolume volume(Vector3Dd(-1, -1, -1), Vector3Dd(1, 1, 1));
    const PreRayHitElement &filter = volume;
    Ray hit(Vector3Dd(-5, 0, 0), Vector3Dd(1, 0, 0));
    Ray miss(Vector3Dd(-5, 3, 0), Vector3Dd(1, 0, 0));
    EXPECT_TRUE(filter.mayIntersect(hit));
    EXPECT_FALSE(filter.mayIntersect(miss));
    EXPECT_TRUE(filter.mayIntersectBefore(hit, 4.0));
    EXPECT_FALSE(filter.mayIntersectBefore(hit, 3.0));
    EXPECT_TRUE(volume.containsPoint(Vector3Dd(0, 0, 0), 0.0));
    BoundingVolumeHierarchy *copy = volume.copy();
    EXPECT_EQ(Vector3Dd(1, 1, 1), copy->axisAlignedExtent().getMax());
    EXPECT_FALSE(copy->isUnbounded());
    delete copy;
    EXPECT_TRUE(AabbBoundingVolume().isUnbounded());
}

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/AabbBinHierarchy.h"

static void addUnitBox(java::ArrayList<AxisAlignedBoundingBox> &boxes,
    java::ArrayList<int> &ids, double x, int id)
{
    boxes.add(AxisAlignedBoundingBox(Vector3Dd(x, -0.5, -0.5), Vector3Dd(x + 1, 0.5, 0.5)));
    ids.add(id);
}

TEST(AabbBinHierarchy, BinsFollowCentroidOrderAlongTheWidestAxis) {
    java::ArrayList<AxisAlignedBoundingBox> boxes;
    java::ArrayList<int> ids;
    // Twenty boxes on the x axis given in reverse order, ids 100..119.
    for (int i = 0; i < 20; i++) addUnitBox(boxes, ids, 3.0 * (19 - i), 100 + i);
    AabbBinHierarchy hierarchy;
    hierarchy.build(boxes, ids);
    EXPECT_EQ(4, hierarchy.getBinCount());
    EXPECT_EQ(20, hierarchy.getMemberCount());
    EXPECT_DOUBLE_EQ(0.0, hierarchy.getBinBounds(0).getMin().x());
    EXPECT_DOUBLE_EQ(13.0, hierarchy.getBinBounds(0).getMax().x());
    EXPECT_DOUBLE_EQ(57.0 + 1.0, hierarchy.getBinBounds(3).getMax().x());
}

TEST(AabbBinHierarchy, QueriesListEveryCrossedPrimitiveAndNoOther) {
    java::ArrayList<AxisAlignedBoundingBox> boxes;
    java::ArrayList<int> ids;
    for (int i = 0; i < 20; i++) addUnitBox(boxes, ids, 3.0 * i, i);
    AabbBinHierarchy hierarchy;
    hierarchy.build(boxes, ids);
    int out[32];
    // A vertical ray through the box at x in [9, 10] crosses only id 3.
    Ray vertical(Vector3Dd(9.5, 5, 0), Vector3Dd(0, -1, 0));
    ASSERT_EQ(1, hierarchy.gatherCandidates(vertical,
        RaySlabReciprocals(vertical.getDirection()), out, 32));
    EXPECT_EQ(3, out[0]);
    // A ray along the row crosses all of them, in bin and member order.
    Ray along(Vector3Dd(-5, 0, 0), Vector3Dd(1, 0, 0));
    ASSERT_EQ(20, hierarchy.gatherCandidates(along,
        RaySlabReciprocals(along.getDirection()), out, 32));
    for (int i = 0; i < 20; i++) EXPECT_EQ(i, out[i]);
    EXPECT_EQ(-1, hierarchy.gatherCandidates(along,
        RaySlabReciprocals(along.getDirection()), out, 5));
    Ray away(Vector3Dd(-5, 0, 0), Vector3Dd(-1, 0, 0));
    EXPECT_EQ(0, hierarchy.gatherCandidates(away,
        RaySlabReciprocals(away.getDirection()), out, 32));
    AabbBinHierarchy empty;
    empty.build(java::ArrayList<AxisAlignedBoundingBox>(), java::ArrayList<int>());
    EXPECT_EQ(0, empty.getBinCount());
    EXPECT_EQ(0, empty.gatherCandidates(along, RaySlabReciprocals(along.getDirection()), out, 32));
}

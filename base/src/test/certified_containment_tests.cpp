#include <gtest/gtest.h>
#include <cmath>
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/PlanarHalfSpace.h"
#include "vsdk/toolkit/environment/geometry/volume/QuadricSurface.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/scene/csg/CsgBodyOperand.h"
#include "vsdk/toolkit/environment/scene/csg/CsgTree.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointClassification.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointAttribution.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointWitness.h"

/*
Certified containment for the basic solid families (stage F-1). Expected
results come from independent closed-form distances; placed cases are also
checked for soundness by sampling the world tolerance ball in long double.
*/

TEST(CertifiedContainmentContract, BoxUsesEuclideanDistanceToItsBoundary) {
    Box box(Vector3Dd(-1,-1,-1), Vector3Dd(1,1,1));
    EXPECT_EQ(GeometryContainment::Inside, box.classifyPointCertified(Vector3Dd(0,0,0), 0.5));
    EXPECT_EQ(GeometryContainment::Boundary, box.classifyPointCertified(Vector3Dd(0.95,0,0), 0.1));
    EXPECT_EQ(GeometryContainment::Boundary, box.classifyPointCertified(Vector3Dd(1.05,0,0), 0.1));
    EXPECT_EQ(GeometryContainment::Outside, box.classifyPointCertified(Vector3Dd(1.2,0,0), 0.1));
    // Corner region: distance sqrt(2)*0.06 = 0.0849 and sqrt(2)*0.08 = 0.1131.
    EXPECT_EQ(GeometryContainment::Boundary, box.classifyPointCertified(Vector3Dd(1.06,1.06,0), 0.1));
    EXPECT_EQ(GeometryContainment::Outside, box.classifyPointCertified(Vector3Dd(1.08,1.08,0), 0.1));
    // A point exactly on a face is a boundary at zero tolerance.
    EXPECT_EQ(GeometryContainment::Boundary, box.classifyPointCertified(Vector3Dd(1,0.5,0), 0));
    EXPECT_EQ(GeometryContainment::Inside, box.classifyPointCertified(Vector3Dd(0.5,0.5,0), 0));
    EXPECT_EQ(GeometryContainment::InvalidInput, box.classifyPointCertified(Vector3Dd(0,0,0), -1));
    // The plain query delegates to the same certificate for positive tolerance.
    EXPECT_EQ(GeometryContainment::Outside, box.classifyPoint(Vector3Dd(1.08,1.08,0), 0.1));
}

TEST(CertifiedContainmentContract, HalfSpaceNormalizesItsCoefficients) {
    PlanarHalfSpace half(2,0,0,-2); // x <= 1
    EXPECT_EQ(GeometryContainment::Inside, half.classifyPointCertified(Vector3Dd(0.8,5,5), 0.1));
    EXPECT_EQ(GeometryContainment::Boundary, half.classifyPointCertified(Vector3Dd(1.05,0,0), 0.1));
    EXPECT_EQ(GeometryContainment::Boundary, half.classifyPointCertified(Vector3Dd(0.95,0,0), 0.1));
    EXPECT_EQ(GeometryContainment::Outside, half.classifyPointCertified(Vector3Dd(1.2,0,0), 0.1));
    PlanarHalfSpace oblique(1,1,0,0); // distance (x+y)/sqrt(2)
    EXPECT_EQ(GeometryContainment::Boundary, oblique.classifyPointCertified(Vector3Dd(0.1,0,0), 0.1));
    EXPECT_EQ(GeometryContainment::Outside, oblique.classifyPointCertified(Vector3Dd(0.1,0.1,0), 0.1));
    EXPECT_EQ(GeometryContainment::InvalidInput, PlanarHalfSpace(0,0,0,1).classifyPointCertified(Vector3Dd(), 0.1));
}

TEST(CertifiedContainmentContract, QuadricProvesSidesAndSignChangesOnly) {
    // x^2/4 + y^2 + z^2 <= 1: semi-axes 2, 1, 1.
    QuadricSurface ellipsoid(Vector3Dd(0.25,1,1), Vector3Dd(), Vector3Dd(), -1);
    EXPECT_EQ(GeometryContainment::Inside, ellipsoid.classifyPointCertified(Vector3Dd(), 0.5));
    EXPECT_EQ(GeometryContainment::Boundary, ellipsoid.classifyPointCertified(Vector3Dd(2.05,0,0), 0.1));
    EXPECT_EQ(GeometryContainment::Boundary, ellipsoid.classifyPointCertified(Vector3Dd(0,0.95,0), 0.1));
    EXPECT_EQ(GeometryContainment::Outside, ellipsoid.classifyPointCertified(Vector3Dd(2.3,0,0), 0.1));
    EXPECT_EQ(GeometryContainment::Outside, ellipsoid.classifyPointCertified(Vector3Dd(0,1.5,0), 0.1));
    // Zero tolerance: sides are proven by the sign of F; the surface itself is not.
    EXPECT_EQ(GeometryContainment::Inside, ellipsoid.classifyPointCertified(Vector3Dd(1.9,0,0), 0));
    EXPECT_EQ(GeometryContainment::Indeterminate, ellipsoid.classifyPointCertified(Vector3Dd(2,0,0), 0));
    // A saddle with a mixed term: xy <= 1.
    QuadricSurface saddle(Vector3Dd(), Vector3Dd(1,0,0), Vector3Dd(), -1);
    EXPECT_EQ(GeometryContainment::Boundary, saddle.classifyPointCertified(Vector3Dd(1,1,0), 0.01));
    EXPECT_EQ(GeometryContainment::Inside, saddle.classifyPointCertified(Vector3Dd(-1,1,0), 0.1));
    // A tangent-only contact (x^2 <= 0) has no sign change and is not claimed.
    QuadricSurface slab(Vector3Dd(1,0,0), Vector3Dd(), Vector3Dd(), 0);
    EXPECT_EQ(GeometryContainment::Indeterminate, slab.classifyPointCertified(Vector3Dd(0.01,0,0), 0.1));
    EXPECT_EQ(GeometryContainment::Outside, slab.classifyPointCertified(Vector3Dd(0.5,0,0), 0.1));
}

class CertifiedContainmentSampler {
public:
    // Signs of the local implicit description at ball samples: -1 inside, +1 outside.
    static long double boxValue(const long double p[3]) {
        long double value = -1e300L;
        for (int i = 0; i < 3; ++i) {
            const long double d = std::fabs(p[i]) - 1.0L;
            if (d > value) value = d;
        }
        return value;
    }
    static long double halfSpaceValue(const long double p[3]) { return p[0] + 2.0L*p[1] - p[2] - 0.5L; }
    static long double ellipsoidValue(const long double p[3]) {
        return 0.25L*p[0]*p[0] + p[1]*p[1] + p[2]*p[2] + 0.5L*p[0]*p[1] - 1.0L;
    }
    static long double value(int family, const long double p[3]) {
        return family == 0 ? boxValue(p) : family == 1 ? halfSpaceValue(p) : ellipsoidValue(p);
    }
    static void local(const Matrix4x4d& inverse, const Vector3Dd& world, long double out[3]) {
        for (int i = 0; i < 3; ++i) {
            out[i] = inverse.get(i,3);
            out[i] += (long double)inverse.get(i,0)*world.x() + (long double)inverse.get(i,1)*world.y() +
                (long double)inverse.get(i,2)*world.z();
        }
    }
    // Returns -1/+1 when every sample has that sign, 0 when both appear.
    static int sampleSign(int family, const Matrix4x4d& inverse, const Vector3Dd& centre, double radius) {
        bool negative = false, positive = false;
        const int count = 96;
        for (int shell = 0; shell <= 2; ++shell) {
            const double r = radius * shell / 2.0;
            for (int k = 0; k < count; ++k) {
                const double z = 1.0 - (2.0*k + 1.0)/count;
                const double ring = std::sqrt(1.0 - z*z);
                const double angle = 2.399963229728653 * k;
                const Vector3Dd q = centre.add(Vector3Dd(ring*std::cos(angle), ring*std::sin(angle), z).multiply(r));
                long double p[3];
                local(inverse, q, p);
                const long double v = value(family, p);
                if (v < 0) negative = true;
                if (v > 0) positive = true;
                if (shell == 0) break;
            }
        }
        return negative && positive ? 0 : negative ? -1 : 1;
    }
};

TEST(CertifiedContainmentContract, PlacedFamiliesAreSoundUnderShearReflectionAndRotation) {
    Box box(Vector3Dd(-1,-1,-1), Vector3Dd(1,1,1));
    PlanarHalfSpace half(1,2,-1,-0.5);
    QuadricSurface ellipsoid(Vector3Dd(0.25,1,1), Vector3Dd(0.5,0,0), Vector3Dd(), -1);
    const Geometry* geometries[3] = {&box, &half, &ellipsoid};
    const Matrix4x4d placements[3] = {
        Matrix4x4d().translation(1,-2,0.5).multiply(Matrix4x4d().scale(3,0.5,1.5)),
        Matrix4x4d().translation(-1,0,2).multiply(Matrix4x4d::identityMatrix().withVal(0,1,0.75)
            .multiply(Matrix4x4d().scale(-1,2,1))),
        Matrix4x4d().axisRotation(0.7, 1, 2, 3).multiply(Matrix4x4d().scale(2,1,0.25))
    };
    int definite = 0, total = 0;
    for (int family = 0; family < 3; ++family) {
        for (int placement = 0; placement < 3; ++placement) {
            SimpleBody body;
            body.setGeometryReference(const_cast<Geometry*>(geometries[family]));
            const Matrix4x4d inverse = placements[placement].inverse();
            ASSERT_EQ(AffinePlacementStatus::Valid, body.setAffinePlacement(placements[placement], inverse));
            for (int step = -40; step <= 40; ++step) {
                const Vector3Dd local(0.05*step, 0.3, -0.2);
                const Vector3Dd world = placements[placement].multiply(local);
                for (int t = 0; t < 2; ++t) {
                    const double tolerance = t == 0 ? 0.05 : 0.2;
                    const GeometryContainment result = body.classifyPointCertified(world, tolerance);
                    ++total;
                    if (result == GeometryContainment::Indeterminate) continue;
                    ASSERT_NE(GeometryContainment::Unsupported, result);
                    ASSERT_NE(GeometryContainment::InvalidInput, result);
                    ++definite;
                    const int sign = CertifiedContainmentSampler::sampleSign(family, inverse, world, tolerance);
                    if (result == GeometryContainment::Inside) EXPECT_EQ(-1, sign) << family << " " << placement << " " << step;
                    if (result == GeometryContainment::Outside) EXPECT_EQ(1, sign) << family << " " << placement << " " << step;
                }
            }
        }
    }
    // The certificates must decide most of the sweep, not merely stay sound.
    EXPECT_GT(definite, total * 3 / 4);
}

TEST(CertifiedContainmentContract, AnisotropicBoxUsesTheTightAxisEnclosure) {
    Box box(2,2,2); // [-1,1]^3
    SimpleBody body;
    body.setGeometryReference(&box);
    body.setPosition(Vector3Dd(10,0,0));
    body.setScale(Vector3Dd(-2,3,1)); // world x in [8,12], y in [-3,3]
    // World distance 0.15 from the x face. The enclosing local ball would have
    // radius about 0.117 against a local distance 0.075; the axis box is exact.
    EXPECT_EQ(GeometryContainment::Outside, body.classifyPointCertified(Vector3Dd(7.85,0,0), 0.1));
    EXPECT_EQ(GeometryContainment::Inside, body.classifyPointCertified(Vector3Dd(8.15,0,0), 0.1));
    EXPECT_EQ(GeometryContainment::Boundary, body.classifyPointCertified(Vector3Dd(8.05,0,0), 0.1));
    // The inner local ball has radius tolerance/3 = 0.02 against a local face
    // distance 0.025; two certified world samples still prove the boundary.
    EXPECT_EQ(GeometryContainment::Boundary, body.classifyPointCertified(Vector3Dd(12.05,0,0), 0.06));
    EXPECT_EQ(GeometryContainment::Outside, body.classifyPointCertified(Vector3Dd(12.07,0,0), 0.06));
}

TEST(CertifiedContainmentContract, CsgLeavesOfEveryBasicFamilyAreCertified) {
    SimpleBody block, cut, rounded;
    block.setGeometry(new Box(Vector3Dd(-1,-1,-1), Vector3Dd(1,1,1)));
    cut.setGeometry(new PlanarHalfSpace(0,0,-1,0)); // z >= 0
    rounded.setGeometry(new QuadricSurface(Vector3Dd(1,1,1), Vector3Dd(), Vector3Dd(), -2.25));
    rounded.setScale(Vector3Dd(1,1,0.5));
    CsgBodyOperand a(block), b(cut), c(rounded);
    CsgTree root(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(root.addOperand(a)); ASSERT_TRUE(root.addOperand(b));
    EXPECT_EQ(GeometryContainment::Inside, CsgPointClassification::classify(root, Vector3Dd(0,0,-0.5), 0.1));
    EXPECT_EQ(GeometryContainment::Outside, CsgPointClassification::classify(root, Vector3Dd(0,0,0.5), 0.1));
    CsgPointWitness witness;
    EXPECT_EQ(GeometryContainment::Boundary,
        CsgPointClassification::classifyWithWitness(root, Vector3Dd(0,0,0.02), 0.05, witness));
    ASSERT_EQ(1, witness.getLength());
    EXPECT_EQ(&b, witness.getOperand(0));
    EXPECT_TRUE(witness.isReversed());
    CsgTree both(CsgOperation::INTERSECTION);
    ASSERT_TRUE(both.addOperand(a)); ASSERT_TRUE(both.addOperand(c));
    // The scaled quadric reaches z = 0.75 on the axis; the box reaches z = 1.
    EXPECT_EQ(GeometryContainment::Boundary, CsgPointClassification::classify(both, Vector3Dd(0,0,0.76), 0.05));
    EXPECT_EQ(GeometryContainment::Outside, CsgPointClassification::classify(both, Vector3Dd(0,0,0.9), 0.05));
    EXPECT_EQ(GeometryContainment::Inside, CsgPointClassification::classify(both, Vector3Dd(0,0,0.5), 0.05));
}

#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/volume/Cone.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPlacedExpression.h"
#include "vsdk/toolkit/environment/scene/csg/CsgSolidBounds.h"

TEST(ConservativeBoundsContract, PlacedBoundsEncloseExactCornersAndStayExactWhenRepresentable) {
    GeometryBounds local;
    ASSERT_TRUE(local.setFinite(Vector3Dd(-0.1,-0.3,0.7), Vector3Dd(0.2,0.9,1.3)));
    const Matrix4x4d placement = Matrix4x4d().translation(0.1,-0.7,0.3).multiply(
        Matrix4x4d::identityMatrix().withVal(0,1,0.3).withVal(1,2,-0.7).withVal(2,0,0.1));
    const GeometryBounds placed = local.transformed(placement);
    ASSERT_EQ(GeometryBounds::Kind::Finite, placed.getKind());
    const double lo[3] = {placed.getMinimum().x(), placed.getMinimum().y(), placed.getMinimum().z()};
    const double hi[3] = {placed.getMaximum().x(), placed.getMaximum().y(), placed.getMaximum().z()};
    for (int corner = 0; corner < 8; ++corner) {
        const double p[3] = {corner & 1 ? 0.2 : -0.1, corner & 2 ? 0.9 : -0.3, corner & 4 ? 1.3 : 0.7};
        for (int row = 0; row < 3; ++row) {
            long double value = placement.get(row,3);
            for (int column = 0; column < 3; ++column) value += (long double)placement.get(row,column) * p[column];
            EXPECT_LE((long double)lo[row], value);
            EXPECT_GE((long double)hi[row], value);
        }
    }
    GeometryBounds unit;
    ASSERT_TRUE(unit.setFinite(Vector3Dd(-1,-1,-1), Vector3Dd(1,1,1)));
    const GeometryBounds shifted = unit.transformed(Matrix4x4d().translation(10,0,0).multiply(Matrix4x4d().scale(2,3,1)));
    EXPECT_EQ(8, shifted.getMinimum().x()); EXPECT_EQ(12, shifted.getMaximum().x());
    EXPECT_EQ(-3, shifted.getMinimum().y()); EXPECT_EQ(3, shifted.getMaximum().y());
}

TEST(ConservativeBoundsContract, IntersectionInflationAndBallExclusionStayConservative) {
    GeometryBounds a, b, unknown, unbounded, empty;
    ASSERT_TRUE(a.setFinite(Vector3Dd(0,0,0), Vector3Dd(2,2,2)));
    ASSERT_TRUE(b.setFinite(Vector3Dd(1,1,1), Vector3Dd(3,3,3)));
    unbounded.setUnbounded(); empty.setEmpty();
    const GeometryBounds both = a.intersecting(b);
    EXPECT_EQ(1, both.getMinimum().x()); EXPECT_EQ(2, both.getMaximum().x());
    EXPECT_EQ(GeometryBounds::Kind::Finite, a.intersecting(unknown).getKind());
    EXPECT_EQ(GeometryBounds::Kind::Finite, unbounded.intersecting(a).getKind());
    EXPECT_EQ(GeometryBounds::Kind::Unsupported, unknown.intersecting(unknown).getKind());
    GeometryBounds far; ASSERT_TRUE(far.setFinite(Vector3Dd(5,5,5), Vector3Dd(6,6,6)));
    EXPECT_EQ(GeometryBounds::Kind::Empty, a.intersecting(far).getKind());
    const GeometryBounds grown = a.inflated(0.1);
    EXPECT_LE(grown.getMinimum().x(), -0.1); EXPECT_GE(grown.getMaximum().x(), 2.1);
    EXPECT_TRUE(a.excludesBall(Vector3Dd(2.2,1,1), 0.1));
    EXPECT_FALSE(a.excludesBall(Vector3Dd(2.05,1,1), 0.1));
    EXPECT_FALSE(a.excludesBall(Vector3Dd(1,1,1), 0.1));
    // Corner distance sqrt(2)*0.08 = 0.113 exceeds 0.1; per-axis checks would not.
    EXPECT_TRUE(a.excludesBall(Vector3Dd(2.08,2.08,1), 0.1));
    EXPECT_TRUE(empty.excludesBall(Vector3Dd(), 1));
    EXPECT_FALSE(unbounded.excludesBall(Vector3Dd(100,0,0), 0.1));
    EXPECT_FALSE(unknown.excludesBall(Vector3Dd(100,0,0), 0.1));
}

TEST(ConservativeBoundsContract, CsgBoundsFollowBooleanOperationsAndPlacement) {
    SimpleBody left, right, plane, mesh;
    left.setGeometry(new Sphere(1));
    right.setGeometry(new Sphere(1)); right.setPosition(Vector3Dd(1.5,0,0));
    plane.setGeometry(new PlanarHalfSpace(1,0,0,0));
    mesh.setGeometry(new TriangleMesh());
    CsgBodyOperand a(left), b(right), c(plane), m(mesh);
    GeometryBounds bounds;
    CsgTree join(CsgOperation::UNION), meet(CsgOperation::INTERSECTION), cut(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(join.addOperand(a)); ASSERT_TRUE(join.addOperand(b));
    ASSERT_TRUE(meet.addOperand(a)); ASSERT_TRUE(meet.addOperand(b)); ASSERT_TRUE(meet.addOperand(c));
    ASSERT_TRUE(cut.addOperand(b)); ASSERT_TRUE(cut.addOperand(c));
    CsgSolidBounds::query(join, bounds);
    ASSERT_EQ(GeometryBounds::Kind::Finite, bounds.getKind());
    EXPECT_LE(bounds.getMinimum().x(), -1); EXPECT_GE(bounds.getMaximum().x(), 2.5);
    CsgSolidBounds::query(meet, bounds);
    ASSERT_EQ(GeometryBounds::Kind::Finite, bounds.getKind());
    EXPECT_LE(bounds.getMinimum().x(), 0.5); EXPECT_GE(bounds.getMaximum().x(), 1);
    EXPECT_LT(bounds.getMaximum().x(), 1.01);
    CsgSolidBounds::query(cut, bounds);
    EXPECT_LE(bounds.getMinimum().x(), 0.5); EXPECT_GE(bounds.getMaximum().x(), 2.5);
    CsgPlacedExpression placed(join);
    const Matrix4x4d forward = Matrix4x4d().translation(0,10,0).multiply(Matrix4x4d().scale(1,2,1));
    ASSERT_EQ(AffinePlacementStatus::Valid, placed.setAffinePlacement(forward, forward.inverse()));
    CsgTree root(CsgOperation::UNION); ASSERT_TRUE(root.addOperand(placed));
    CsgSolidBounds::query(root, bounds);
    EXPECT_LE(bounds.getMinimum().y(), 8); EXPECT_GE(bounds.getMaximum().y(), 12);
    // A surface has no solid bounds; a union with it stays unknown.
    CsgTree withSurface(CsgOperation::UNION);
    ASSERT_TRUE(withSurface.addOperand(a)); ASSERT_TRUE(withSurface.addOperand(m));
    CsgSolidBounds::query(withSurface, bounds);
    EXPECT_EQ(GeometryBounds::Kind::Unsupported, bounds.getKind());
}

TEST(ConservativeBoundsContract, LeafCullingDecidesOnlyProvenDisjointSolids) {
    SimpleBody cone, mesh;
    cone.setGeometry(new Cone(1,1,1));
    mesh.setGeometry(new TriangleMesh());
    CsgBodyOperand c(cone), m(mesh);
    CsgTree solid(CsgOperation::UNION), surface(CsgOperation::UNION);
    ASSERT_TRUE(solid.addOperand(c)); ASSERT_TRUE(surface.addOperand(m));
    // The cone has no certified containment, but its certified bounds prove
    // that a distant ball is outside.
    EXPECT_EQ(GeometryContainment::Outside, CsgPointClassification::classify(solid, Vector3Dd(10,0,0), 0.1));
    EXPECT_EQ(GeometryContainment::Unsupported, CsgPointClassification::classify(solid, Vector3Dd(0,0,0.5), 0.1));
    // An open surface never acquires an exterior from its bounds.
    EXPECT_EQ(GeometryContainment::Unsupported, CsgPointClassification::classify(surface, Vector3Dd(10,0,0), 0.1));
}

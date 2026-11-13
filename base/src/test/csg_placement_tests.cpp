#include <gtest/gtest.h>
#include "java/util/ArrayList.txx"
#include "java/lang/Math.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/scene/AffineToleranceMap.h"
#include "UncertifiedTestBody.h"
#include "vsdk/toolkit/environment/scene/csg/CsgBodyOperand.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPlacedExpression.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointClassification.h"
#include "vsdk/toolkit/environment/scene/csg/CsgTree.h"

TEST(CsgPlacedComposition, PreservesWorldToleranceUnderShearAndReflection) {
    SimpleBody sphere;
    sphere.setGeometry(new Sphere(1));
    CsgBodyOperand leaf(sphere);
    CsgTree child(CsgOperation::UNION);
    ASSERT_TRUE(child.addOperand(leaf));
    CsgPlacedExpression placed(child);
    const Matrix4x4d shear=Matrix4x4d::identityMatrix().withVal(0,1,0.5);
    const Matrix4x4d unshear=Matrix4x4d::identityMatrix().withVal(0,1,-0.5);
    const Matrix4x4d forward=Matrix4x4d().translation(4,-1,2).multiply(shear).multiply(Matrix4x4d().scale(-2,1,0.5));
    const Matrix4x4d inverse=Matrix4x4d().scale(-0.5,1,2).multiply(unshear).multiply(Matrix4x4d().translation(-4,1,-2));
    ASSERT_EQ(AffinePlacementStatus::Valid,placed.setAffinePlacement(forward,inverse));
    CsgTree root(CsgOperation::UNION);
    ASSERT_TRUE(root.addOperand(placed));
    EXPECT_EQ(GeometryContainment::Inside,CsgPointClassification::classify(root,forward.multiply(Vector3Dd()),0.05));
    EXPECT_EQ(GeometryContainment::Inside,CsgPointClassification::classify(root,forward.multiply(Vector3Dd()),0));
    EXPECT_EQ(GeometryContainment::Outside,CsgPointClassification::classify(root,forward.multiply(Vector3Dd(2,0,0)),0.05));
    EXPECT_EQ(GeometryContainment::Outside,CsgPointClassification::classify(root,forward.multiply(Vector3Dd(2,0,0)),0));
    EXPECT_EQ(GeometryContainment::Indeterminate,CsgPointClassification::classify(root,forward.multiply(Vector3Dd(1,0,0)),0));
    EXPECT_EQ(GeometryContainment::Boundary,CsgPointClassification::classify(root,forward.multiply(Vector3Dd(1.01,0,0)),0.1));
    EXPECT_EQ(AffinePlacementStatus::Invalid,placed.setAffinePlacement(Matrix4x4d().scale(0,1,1),Matrix4x4d()));
    EXPECT_EQ(GeometryContainment::Inside,CsgPointClassification::classify(root,forward.multiply(Vector3Dd()),0.05));
    EXPECT_FALSE(child.addOperand(placed));
}

TEST(CsgPlacedComposition, NestedDifferenceUsesOrderedOrientation) {
    SimpleBody big, small;
    big.setGeometry(new Sphere(2)); small.setGeometry(new Sphere(1));
    CsgBodyOperand a(big), b(small);
    CsgTree difference(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(difference.addOperand(a)); ASSERT_TRUE(difference.addOperand(b));
    CsgPlacedExpression placed(difference);
    const Matrix4x4d forward=Matrix4x4d().translation(10,0,0).multiply(Matrix4x4d().scale(2,1,1));
    const Matrix4x4d inverse=Matrix4x4d().scale(0.5,1,1).multiply(Matrix4x4d().translation(-10,0,0));
    ASSERT_EQ(AffinePlacementStatus::Valid,placed.setAffinePlacement(forward,inverse));
    CsgTree root(CsgOperation::INTERSECTION);
    ASSERT_TRUE(root.addOperand(placed));
    EXPECT_EQ(GeometryContainment::Outside,CsgPointClassification::classify(root,Vector3Dd(10,0,0),0.05));
    EXPECT_EQ(GeometryContainment::Inside,CsgPointClassification::classify(root,Vector3Dd(13,0,0),0.05));
    EXPECT_EQ(GeometryContainment::Outside,CsgPointClassification::classify(root,Vector3Dd(15,0,0),0.05));
}

TEST(AffineToleranceMap, EnclosesInverseImagesAndDoesNotMutateOnFailure) {
    const Matrix4x4d forward=Matrix4x4d().translation(4,-1,2).multiply(
        Matrix4x4d::identityMatrix().withVal(0,1,0.5)).multiply(Matrix4x4d().scale(-2,1,0.5));
    const Matrix4x4d inverse=Matrix4x4d().scale(-0.5,1,2).multiply(
        Matrix4x4d::identityMatrix().withVal(0,1,-0.5)).multiply(Matrix4x4d().translation(-4,1,-2));
    const Vector3Dd point(4.3,-0.7,2.1);
    Vector3Dd center;
    double bound=0;
    double inner=-1;
    ASSERT_EQ(AffinePlacementStatus::Valid,AffineToleranceMap::map(forward,point,0.4,center,bound,&inner));
    ASSERT_GT(inner,0);
    for (int x=-1;x<=1;++x) for (int y=-1;y<=1;++y) for (int z=-1;z<=1;++z) {
        const double length=java::Math::sqrt(double(x*x+y*y+z*z));
        if (length==0) continue;
        const Vector3Dd sample=point.add(Vector3Dd(x,y,z).multiply(0.4/length));
        EXPECT_LE(inverse.multiply(sample).subtract(center).length(),bound);
        const Vector3Dd childSample=center.add(Vector3Dd(x,y,z).multiply(inner/length));
        EXPECT_LE(forward.multiply(childSample).subtract(point).length(),0.4);
    }
    center=Vector3Dd(7,8,9); bound=42;
    EXPECT_EQ(AffinePlacementStatus::Indeterminate,AffineToleranceMap::map(Matrix4x4d().scale(0,1,1),point,0.4,center,bound));
    EXPECT_DOUBLE_EQ(7,center.x()); EXPECT_DOUBLE_EQ(42,bound);
    EXPECT_EQ(AffinePlacementStatus::Invalid,AffineToleranceMap::map(forward,point,-1,center,bound));
    EXPECT_DOUBLE_EQ(7,center.x()); EXPECT_DOUBLE_EQ(42,bound);
}

#include "vsdk/toolkit/environment/scene/csg/CsgPointWitness.h"
TEST(CsgPointWitnessContract, RecordsDecisivePathAndDifferenceOrientation) {
    SimpleBody outer, inner;
    outer.setGeometry(new Sphere(2)); inner.setGeometry(new Sphere(1));
    CsgBodyOperand a(outer), b(inner);
    CsgTree difference(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(difference.addOperand(a)); ASSERT_TRUE(difference.addOperand(b));
    CsgPlacedExpression placed(difference);
    const Matrix4x4d forward=Matrix4x4d().translation(3,0,0);
    ASSERT_EQ(AffinePlacementStatus::Valid,placed.setAffinePlacement(forward,Matrix4x4d().translation(-3,0,0)));
    CsgTree root(CsgOperation::UNION);
    ASSERT_TRUE(root.addOperand(placed));
    CsgPointWitness witness;
    ASSERT_EQ(GeometryContainment::Outside,CsgPointClassification::classifyWithWitness(root,Vector3Dd(3,0,0),0.1,witness));
    ASSERT_EQ(2,witness.getLength());
    EXPECT_EQ(&placed,witness.getOperand(0)); EXPECT_EQ(&b,witness.getOperand(1));
    EXPECT_TRUE(witness.isReversed());
    EXPECT_EQ(nullptr,witness.getOperand(-1)); EXPECT_EQ(nullptr,witness.getOperand(2));
    // Inside a difference requires both operands, so a single-leaf proof is unavailable.
    EXPECT_EQ(GeometryContainment::Inside,CsgPointClassification::classifyWithWitness(root,Vector3Dd(4.5,0,0),0.05,witness));
    EXPECT_FALSE(witness.isAvailable()); EXPECT_FALSE(witness.isReversed());
    EXPECT_EQ(GeometryContainment::InvalidInput,CsgPointClassification::classifyWithWitness(root,Vector3Dd(),-1,witness));
    EXPECT_FALSE(witness.isAvailable());
}

TEST(CsgPointWitnessContract, CertifiesOnlyExposedBoundaries) {
    SimpleBody outer, inner, distant;
    outer.setGeometry(new Sphere(2)); inner.setGeometry(new Sphere(1)); distant.setGeometry(new Sphere(1));
    distant.setPosition(Vector3Dd(10,0,0));
    CsgBodyOperand a(outer), b(inner), c(distant);
    CsgTree united(CsgOperation::UNION), intersected(CsgOperation::INTERSECTION), difference(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(united.addOperand(a)); ASSERT_TRUE(united.addOperand(c));
    ASSERT_TRUE(intersected.addOperand(a)); ASSERT_TRUE(intersected.addOperand(b));
    ASSERT_TRUE(difference.addOperand(a)); ASSERT_TRUE(difference.addOperand(b));
    CsgPointWitness witness;
    EXPECT_EQ(GeometryContainment::Boundary,CsgPointClassification::classifyWithWitness(united,Vector3Dd(2,0,0),0.01,witness));
    ASSERT_EQ(1,witness.getLength()); EXPECT_EQ(&a,witness.getOperand(0)); EXPECT_FALSE(witness.isReversed());
    EXPECT_EQ(GeometryContainment::Boundary,CsgPointClassification::classifyWithWitness(intersected,Vector3Dd(1,0,0),0.01,witness));
    ASSERT_EQ(1,witness.getLength()); EXPECT_EQ(&b,witness.getOperand(0)); EXPECT_FALSE(witness.isReversed());
    EXPECT_EQ(GeometryContainment::Boundary,CsgPointClassification::classifyWithWitness(difference,Vector3Dd(1,0,0),0.01,witness));
    ASSERT_EQ(1,witness.getLength()); EXPECT_EQ(&b,witness.getOperand(0)); EXPECT_TRUE(witness.isReversed());
    CsgBodyOperand repeatedInner(inner);
    ASSERT_TRUE(difference.addOperand(repeatedInner));
    EXPECT_EQ(GeometryContainment::Boundary,CsgPointClassification::classifyWithWitness(difference,Vector3Dd(1,0,0),0.01,witness));
    ASSERT_EQ(1,witness.getLength()); EXPECT_EQ(&b,witness.getOperand(0)); EXPECT_TRUE(witness.isReversed());
    CsgTree coincident(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(coincident.addOperand(b)); ASSERT_TRUE(coincident.addOperand(b));
    EXPECT_EQ(GeometryContainment::Outside,CsgPointClassification::classifyWithWitness(coincident,Vector3Dd(1,0,0),0.01,witness));
    EXPECT_FALSE(witness.isAvailable());
}

TEST(CsgPlacedComposition, TraversesMultiplePlacementsAndRetainsWitnessPath) {
    SimpleBody sphere;
    sphere.setGeometry(new Sphere(1));
    CsgBodyOperand leaf(sphere);
    CsgTree inner(CsgOperation::UNION);
    ASSERT_TRUE(inner.addOperand(leaf));
    CsgPlacedExpression first(inner);
    ASSERT_EQ(AffinePlacementStatus::Valid,first.setAffinePlacement(
        Matrix4x4d().translation(2,0,0),Matrix4x4d().translation(-2,0,0)));
    CsgTree middle(CsgOperation::UNION);
    ASSERT_TRUE(middle.addOperand(first));
    CsgPlacedExpression second(middle);
    ASSERT_EQ(AffinePlacementStatus::Valid,second.setAffinePlacement(
        Matrix4x4d().scale(-2,1,1),Matrix4x4d().scale(-0.5,1,1)));
    CsgTree root(CsgOperation::UNION);
    ASSERT_TRUE(root.addOperand(second));
    CsgPointWitness witness;
    EXPECT_EQ(GeometryContainment::Inside,CsgPointClassification::classifyWithWitness(root,Vector3Dd(-4,0,0),0.1,witness));
    ASSERT_EQ(3,witness.getLength());
    EXPECT_EQ(&second,witness.getOperand(0)); EXPECT_EQ(&first,witness.getOperand(1)); EXPECT_EQ(&leaf,witness.getOperand(2));
    EXPECT_EQ(GeometryContainment::Outside,CsgPointClassification::classify(root,Vector3Dd(2,0,0),0.1));
    EXPECT_EQ(GeometryContainment::Boundary,CsgPointClassification::classify(root,Vector3Dd(-6.01,0,0),0.1));
}

TEST(AffineToleranceMap, NearSingularMappingPreservesFailureOutputs) {
    // The exact determinant of these binary64 entries is zero, but the rounded
    // products cannot decide its sign.
    const Matrix4x4d nearlySingular=Matrix4x4d::identityMatrix().withVal(0,0,0.1).withVal(0,1,0.3)
        .withVal(1,0,0.2).withVal(1,1,0.6);
    Vector3Dd point(7,8,9); double tolerance=42;
    EXPECT_EQ(AffinePlacementStatus::Indeterminate,AffineToleranceMap::map(nearlySingular,Vector3Dd(),0.1,point,tolerance));
    EXPECT_DOUBLE_EQ(7,point.x()); EXPECT_DOUBLE_EQ(42,tolerance);
    // An exactly representable determinant 2^-52 is decided, however small.
    const Matrix4x4d tiny=Matrix4x4d::identityMatrix().withVal(0,0,1).withVal(0,1,1)
        .withVal(1,0,1).withVal(1,1,java::Math::nextAfter(1,2));
    EXPECT_EQ(AffinePlacementStatus::Valid,AffineToleranceMap::map(tiny,Vector3Dd(),0.1,point,tolerance));
}

TEST(AffineToleranceMap, EnclosesSamplesAcrossScalesAndShears) {
    const double scales[4]={0.01,0.5,2,100};
    for (int i=0;i<4;++i) for (int j=0;j<4;++j) {
        const double sx=scales[i], sy=scales[j], sz=(i%2 ? -1.5 : 1.5);
        const Matrix4x4d forward=Matrix4x4d().translation(5,-3,2).multiply(
            Matrix4x4d::identityMatrix().withVal(0,1,0.75).withVal(1,2,-0.25)).multiply(
            Matrix4x4d().scale(sx,sy,sz));
        const Matrix4x4d inverse=Matrix4x4d().scale(1/sx,1/sy,1/sz).multiply(
            Matrix4x4d::identityMatrix().withVal(0,1,-0.75).withVal(1,2,0.25).withVal(0,2,-0.1875)).multiply(
            Matrix4x4d().translation(-5,3,-2));
        const Vector3Dd point(5.25,-3.5,2.125);
        Vector3Dd center; double outer=0, inner=-1;
        ASSERT_EQ(AffinePlacementStatus::Valid,AffineToleranceMap::map(forward,point,0.25,center,outer,&inner));
        ASSERT_GT(inner,0);
        for (int x=-1;x<=1;++x) for (int y=-1;y<=1;++y) for (int z=-1;z<=1;++z) {
            const double length=java::Math::sqrt(double(x*x+y*y+z*z));
            if (length==0) continue;
            const Vector3Dd direction=Vector3Dd(x,y,z).multiply(1/length);
            const Vector3Dd parentSample=point.add(direction.multiply(0.25));
            EXPECT_LE(inverse.multiply(parentSample).subtract(center).length(),outer);
            const Vector3Dd childSample=center.add(direction.multiply(inner));
            EXPECT_LE(forward.multiply(childSample).subtract(point).length(),0.25);
        }
    }
}

#include <fenv.h>
TEST(AffineToleranceMap, RejectsUnsupportedRoundingWithoutChangingOutputs) {
    const int original=fegetround();
    if (fesetround(FE_DOWNWARD)!=0) GTEST_SKIP() << "Rounding mode cannot be changed";
    Vector3Dd center(7,8,9); double outer=42, inner=17;
    const AffinePlacementStatus status=AffineToleranceMap::map(Matrix4x4d().translation(1,2,3),
        Vector3Dd(4,5,6),0.1,center,outer,&inner);
    fesetround(original);
    EXPECT_EQ(AffinePlacementStatus::Unsupported,status);
    EXPECT_DOUBLE_EQ(7,center.x()); EXPECT_DOUBLE_EQ(42,outer); EXPECT_DOUBLE_EQ(17,inner);
}

TEST(CsgPlacedComposition, RotatedBoundaryKeepsItsWitness) {
    SimpleBody sphere;
    sphere.setGeometry(new Sphere(1));
    CsgBodyOperand leaf(sphere);
    CsgTree child(CsgOperation::UNION);
    ASSERT_TRUE(child.addOperand(leaf));
    CsgPlacedExpression placed(child);
    const Matrix4x4d rotation=Matrix4x4d::identityMatrix().withVal(0,0,0).withVal(0,1,-1)
        .withVal(1,0,1).withVal(1,1,0);
    const Matrix4x4d inverseRotation=rotation.transpose();
    const Matrix4x4d forward=Matrix4x4d().translation(2,3,0).multiply(rotation).multiply(Matrix4x4d().scale(2,1,1));
    const Matrix4x4d inverse=Matrix4x4d().scale(0.5,1,1).multiply(inverseRotation).multiply(Matrix4x4d().translation(-2,-3,0));
    ASSERT_EQ(AffinePlacementStatus::Valid,placed.setAffinePlacement(forward,inverse));
    CsgTree root(CsgOperation::UNION);
    ASSERT_TRUE(root.addOperand(placed));
    CsgPointWitness witness;
    EXPECT_EQ(GeometryContainment::Boundary,CsgPointClassification::classifyWithWitness(
        root,forward.multiply(Vector3Dd(1,0,0)),0.1,witness));
    ASSERT_EQ(2,witness.getLength());
    EXPECT_EQ(&placed,witness.getOperand(0)); EXPECT_EQ(&leaf,witness.getOperand(1));
    EXPECT_FALSE(witness.isReversed());
}

#include "CsgTestOperand.h"
TEST(CsgPlacedComposition, InnerProbeDoesNotHideErrors) {
    CsgTestOperand invalid(GeometryContainment::Indeterminate,GeometryContainment::InvalidInput,0.08);
    CsgTestOperand unsupported(GeometryContainment::Indeterminate,GeometryContainment::Unsupported,0.08);
    CsgTree invalidChild(CsgOperation::UNION), unsupportedChild(CsgOperation::UNION);
    ASSERT_TRUE(invalidChild.addOperand(invalid)); ASSERT_TRUE(unsupportedChild.addOperand(unsupported));
    CsgPlacedExpression placedInvalid(invalidChild), placedUnsupported(unsupportedChild);
    const Matrix4x4d forward=Matrix4x4d().scale(2,1,1), inverse=Matrix4x4d().scale(0.5,1,1);
    ASSERT_EQ(AffinePlacementStatus::Valid,placedInvalid.setAffinePlacement(forward,inverse));
    ASSERT_EQ(AffinePlacementStatus::Valid,placedUnsupported.setAffinePlacement(forward,inverse));
    CsgTree rootInvalid(CsgOperation::UNION), rootUnsupported(CsgOperation::UNION);
    ASSERT_TRUE(rootInvalid.addOperand(placedInvalid)); ASSERT_TRUE(rootUnsupported.addOperand(placedUnsupported));
    EXPECT_EQ(GeometryContainment::InvalidInput,CsgPointClassification::classify(rootInvalid,Vector3Dd(),0.1));
    EXPECT_EQ(GeometryContainment::Unsupported,CsgPointClassification::classify(rootUnsupported,Vector3Dd(),0.1));
}

#include "vsdk/toolkit/environment/scene/csg/CsgPointProof.h"
TEST(CsgPointProofContract, RecordsConjunctiveLeafFactsAndBoundaryOrientation) {
    SimpleBody outer, inner, distant;
    outer.setGeometry(new Sphere(2)); inner.setGeometry(new Sphere(1)); distant.setGeometry(new Sphere(1));
    distant.setPosition(Vector3Dd(10,0,0));
    CsgBodyOperand a(outer), b(inner), c(distant);
    CsgTree united(CsgOperation::UNION), intersection(CsgOperation::INTERSECTION), difference(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(united.addOperand(a)); ASSERT_TRUE(united.addOperand(c));
    ASSERT_TRUE(intersection.addOperand(a)); ASSERT_TRUE(intersection.addOperand(b));
    ASSERT_TRUE(difference.addOperand(a)); ASSERT_TRUE(difference.addOperand(b));
    CsgPointProof proof;
    EXPECT_EQ(GeometryContainment::Outside,CsgPointClassification::classifyWithProof(united,Vector3Dd(5,0,0),0.1,proof));
    ASSERT_TRUE(proof.isAvailable()); ASSERT_EQ(2,proof.getCount());
    EXPECT_EQ(GeometryContainment::Outside,proof.getEvidence(0)->getClassification());
    EXPECT_EQ(GeometryContainment::Outside,proof.getEvidence(1)->getClassification());
    EXPECT_EQ(&a,proof.getEvidence(0)->getPath().getOperand(0));
    EXPECT_EQ(&c,proof.getEvidence(1)->getPath().getOperand(0));
    EXPECT_DOUBLE_EQ(5,proof.getEvidence(0)->getPoint().x());
    EXPECT_DOUBLE_EQ(0.1,proof.getEvidence(0)->getTolerance());
    EXPECT_EQ(GeometryContainment::Inside,CsgPointClassification::classifyWithProof(intersection,Vector3Dd(),0.1,proof));
    ASSERT_EQ(2,proof.getCount());
    EXPECT_EQ(GeometryContainment::Inside,proof.getEvidence(0)->getClassification());
    EXPECT_EQ(GeometryContainment::Inside,proof.getEvidence(1)->getClassification());
    EXPECT_EQ(GeometryContainment::Boundary,CsgPointClassification::classifyWithProof(difference,Vector3Dd(1,0,0),0.01,proof));
    ASSERT_EQ(2,proof.getCount());
    EXPECT_EQ(GeometryContainment::Boundary,proof.getEvidence(1)->getClassification());
    EXPECT_TRUE(proof.getEvidence(1)->getPath().isReversed());
    EXPECT_EQ(nullptr,proof.getEvidence(2));
    EXPECT_EQ(GeometryContainment::InvalidInput,CsgPointClassification::classifyWithProof(difference,Vector3Dd(),-1,proof));
    EXPECT_FALSE(proof.isAvailable());
}
TEST(CsgPointProofContract, SimplifiesEqualSpheresButNotMerelyNearbySurfaces) {
    SimpleBody solid, equalButDistinct, almostEqual;
    solid.setGeometry(new Sphere(1)); equalButDistinct.setGeometry(new Sphere(1));
    almostEqual.setGeometry(new Sphere(1)); almostEqual.setPosition(Vector3Dd(0.0001,0,0));
    CsgBodyOperand a(solid), alias(solid), other(equalButDistinct), nearOther(almostEqual);
    CsgTree united(CsgOperation::UNION), intersection(CsgOperation::INTERSECTION), difference(CsgOperation::DIFFERENCE), unknown(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(united.addOperand(a)); ASSERT_TRUE(united.addOperand(alias));
    ASSERT_TRUE(intersection.addOperand(a)); ASSERT_TRUE(intersection.addOperand(alias));
    ASSERT_TRUE(difference.addOperand(a)); ASSERT_TRUE(difference.addOperand(alias));
    ASSERT_TRUE(unknown.addOperand(a)); ASSERT_TRUE(unknown.addOperand(nearOther));
    CsgPointProof proof;
    EXPECT_EQ(GeometryContainment::Boundary,CsgPointClassification::classifyWithProof(united,Vector3Dd(1,0,0),0.01,proof));
    ASSERT_EQ(2,proof.getCount());
    EXPECT_EQ(GeometryContainment::Boundary,CsgPointClassification::classify(intersection,Vector3Dd(1,0,0),0.01));
    EXPECT_EQ(GeometryContainment::Outside,CsgPointClassification::classifyWithProof(difference,Vector3Dd(1,0,0),0.01,proof));
    ASSERT_EQ(2,proof.getCount()); EXPECT_TRUE(proof.getEvidence(1)->getPath().isReversed());
    CsgTree equalGeometry(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(equalGeometry.addOperand(a)); ASSERT_TRUE(equalGeometry.addOperand(other));
    EXPECT_EQ(GeometryContainment::Outside,CsgPointClassification::classifyWithProof(equalGeometry,Vector3Dd(1,0,0),0.01,proof));
    ASSERT_EQ(2,proof.getCount());
    EXPECT_EQ(GeometryContainment::Indeterminate,CsgPointClassification::classifyWithProof(unknown,Vector3Dd(1,0,0),0.01,proof));
    EXPECT_FALSE(proof.isAvailable());
    CsgTestOperand uncertain(GeometryContainment::Indeterminate);
    CsgTree selfWithUnknown(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(selfWithUnknown.addOperand(a)); ASSERT_TRUE(selfWithUnknown.addOperand(alias));
    ASSERT_TRUE(selfWithUnknown.addOperand(uncertain));
    EXPECT_EQ(GeometryContainment::Outside,CsgPointClassification::classifyWithProof(selfWithUnknown,Vector3Dd(1,0,0),0.01,proof));
    ASSERT_EQ(2,proof.getCount());
}

#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayLeafBatch.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayLeafQuery.h"
TEST(CsgRayLeafQueryContract, CollectsPlacedDifferenceAndPreservesOrientation) {
    SimpleBody outer, inner;
    outer.setGeometry(new Sphere(2)); inner.setGeometry(new Sphere(1));
    CsgBodyOperand a(outer), b(inner);
    CsgTree difference(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(difference.addOperand(a)); ASSERT_TRUE(difference.addOperand(b));
    CsgPlacedExpression placed(difference);
    const Matrix4x4d forward=Matrix4x4d().translation(3,0,0).multiply(Matrix4x4d().scale(2,1,1));
    const Matrix4x4d inverse=Matrix4x4d().scale(0.5,1,1).multiply(Matrix4x4d().translation(-3,0,0));
    ASSERT_EQ(AffinePlacementStatus::Valid,placed.setAffinePlacement(forward,inverse));
    CsgTree root(CsgOperation::UNION);
    ASSERT_TRUE(root.addOperand(placed));
    CsgRayLeafBatch batch;
    ASSERT_EQ(RaySolidQueryStatus::Supported,CsgRayLeafQuery::query(root,
        Ray::fromAffine(Vector3Dd(-5,0,0),Vector3Dd(1,0,0)),RayCrossingQuery(0,20),batch));
    ASSERT_EQ(2,batch.size());
    const CsgRayLeaf* first=batch.get(0); const CsgRayLeaf* second=batch.get(1);
    ASSERT_EQ(2,first->getPath().getLength()); ASSERT_EQ(2,second->getPath().getLength());
    EXPECT_EQ(&placed,first->getPath().getOperand(0)); EXPECT_EQ(&a,first->getPath().getOperand(1));
    EXPECT_EQ(&b,second->getPath().getOperand(1));
    EXPECT_FALSE(first->getPath().isReversed()); EXPECT_TRUE(second->getPath().isReversed());
    const RaySolidClassification& out=first->getClassification();
    const RaySolidClassification& in=second->getClassification();
    ASSERT_EQ(2,out.boundaries.size()); ASSERT_EQ(2,in.boundaries.size());
    ASSERT_EQ(3,out.insideSegments.size()); ASSERT_EQ(3,in.insideSegments.size());
    EXPECT_DOUBLE_EQ(4,out.boundaries[0].t); EXPECT_DOUBLE_EQ(12,out.boundaries[1].t);
    EXPECT_DOUBLE_EQ(6,in.boundaries[0].t); EXPECT_DOUBLE_EQ(10,in.boundaries[1].t);
    EXPECT_FALSE(out.insideSegments[0]); EXPECT_TRUE(out.insideSegments[1]); EXPECT_FALSE(out.insideSegments[2]);
    EXPECT_FALSE(in.insideSegments[0]); EXPECT_TRUE(in.insideSegments[1]); EXPECT_FALSE(in.insideSegments[2]);
    EXPECT_FALSE(out.boundaries[0].normalReversed); EXPECT_TRUE(in.boundaries[0].normalReversed);
    ASSERT_EQ(1,out.boundaries[0].getInstanceCount());
    ASSERT_EQ(1,first->getPlacementCount());
    const CsgRayPlacement* parent=first->getPlacement(0);
    ASSERT_NE(nullptr,parent);
    EXPECT_EQ(&placed,parent->operand);
    EXPECT_DOUBLE_EQ(3,parent->childToParent.multiply(Vector3Dd()).x());
}
TEST(CsgRayLeafQueryContract, KeepsCoincidentLeavesAndClearsResultsOnFailure) {
    SimpleBody sphere; sphere.setGeometry(new Sphere(1));
    CsgBodyOperand a(sphere), alias(sphere);
    CsgTree equal(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(equal.addOperand(a)); ASSERT_TRUE(equal.addOperand(alias));
    CsgRayLeafBatch batch;
    const Ray ray=Ray::fromAffine(Vector3Dd(-3,0,0),Vector3Dd(1,0,0));
    ASSERT_EQ(RaySolidQueryStatus::Supported,CsgRayLeafQuery::query(equal,ray,RayCrossingQuery(0,10),batch));
    ASSERT_EQ(2,batch.size());
    EXPECT_DOUBLE_EQ(batch.get(0)->getClassification().boundaries[0].t,batch.get(1)->getClassification().boundaries[0].t);
    EXPECT_FALSE(batch.get(0)->getPath().isReversed()); EXPECT_TRUE(batch.get(1)->getPath().isReversed());
    CsgTestOperand unsupported(GeometryContainment::Indeterminate);
    CsgTree partial(CsgOperation::UNION);
    ASSERT_TRUE(partial.addOperand(a)); ASSERT_TRUE(partial.addOperand(unsupported));
    EXPECT_EQ(RaySolidQueryStatus::Unsupported,CsgRayLeafQuery::query(partial,ray,RayCrossingQuery(0,10),batch));
    EXPECT_EQ(0,batch.size());
    unsupported.setMalformedRay(true);
    EXPECT_EQ(RaySolidQueryStatus::Indeterminate,CsgRayLeafQuery::query(partial,ray,RayCrossingQuery(0,10),batch));
    EXPECT_EQ(0,batch.size());
    EXPECT_EQ(RaySolidQueryStatus::InvalidInput,CsgRayLeafQuery::query(equal,ray,RayCrossingQuery(1,1),batch));
    EXPECT_EQ(0,batch.size());
}

TEST(CsgRayLeafQueryContract, NestedDifferencesPreserveParityAndLeafParameter) {
    SimpleBody outer, middle, inner;
    outer.setGeometry(new Sphere(3)); middle.setGeometry(new Sphere(2)); inner.setGeometry(new Sphere(1));
    CsgBodyOperand a(outer), b(middle), c(inner);
    CsgTree nested(CsgOperation::DIFFERENCE), root(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(nested.addOperand(b)); ASSERT_TRUE(nested.addOperand(c));
    ASSERT_TRUE(root.addOperand(a)); ASSERT_TRUE(root.addOperand(nested));
    CsgRayLeafBatch batch;
    ASSERT_EQ(RaySolidQueryStatus::Supported,CsgRayLeafQuery::query(root,
        Ray::fromAffine(Vector3Dd(-5,0,0),Vector3Dd(2,0,0)),RayCrossingQuery(0,10),batch));
    ASSERT_EQ(3,batch.size());
    EXPECT_FALSE(batch.get(0)->getPath().isReversed());
    EXPECT_TRUE(batch.get(1)->getPath().isReversed());
    EXPECT_FALSE(batch.get(2)->getPath().isReversed());
    EXPECT_TRUE(batch.get(1)->getClassification().boundaries[0].normalReversed);
    EXPECT_FALSE(batch.get(2)->getClassification().boundaries[0].normalReversed);
    EXPECT_DOUBLE_EQ(1,batch.get(0)->getClassification().boundaries[0].t);
    EXPECT_DOUBLE_EQ(1.5,batch.get(1)->getClassification().boundaries[0].t);
    EXPECT_DOUBLE_EQ(2,batch.get(2)->getClassification().boundaries[0].t);
}

#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/PlanarHalfSpace.h"
#include "vsdk/toolkit/environment/geometry/volume/QuadricSurface.h"
TEST(CsgRayLeafQueryContract, CollectsBasicSolidFamiliesAndEndpointEvents) {
    SimpleBody box, halfspace, quadric;
    box.setGeometry(new Box(2,2,2));
    halfspace.setGeometry(new PlanarHalfSpace(1,0,0,0));
    quadric.setGeometry(new QuadricSurface(Vector3Dd(1,1,1),Vector3Dd(),Vector3Dd(),-1));
    CsgBodyOperand a(box), b(halfspace), c(quadric);
    CsgTree root(CsgOperation::INTERSECTION);
    ASSERT_TRUE(root.addOperand(a)); ASSERT_TRUE(root.addOperand(b)); ASSERT_TRUE(root.addOperand(c));
    CsgRayLeafBatch batch;
    ASSERT_EQ(RaySolidQueryStatus::Supported,CsgRayLeafQuery::query(root,
        Ray::fromAffine(Vector3Dd(-3,0,0),Vector3Dd(1,0,0)),RayCrossingQuery(2,4),batch));
    ASSERT_EQ(3,batch.size());
    ASSERT_EQ(2,batch.get(0)->getClassification().boundaries.size());
    ASSERT_EQ(1,batch.get(1)->getClassification().boundaries.size());
    ASSERT_EQ(2,batch.get(2)->getClassification().boundaries.size());
    EXPECT_DOUBLE_EQ(2,batch.get(0)->getClassification().boundaries[0].t);
    EXPECT_DOUBLE_EQ(4,batch.get(0)->getClassification().boundaries[1].t);
    EXPECT_DOUBLE_EQ(3,batch.get(1)->getClassification().boundaries[0].t);
    EXPECT_DOUBLE_EQ(2,batch.get(2)->getClassification().boundaries[0].t);
    EXPECT_DOUBLE_EQ(4,batch.get(2)->getClassification().boundaries[1].t);
}

#include "vsdk/toolkit/environment/scene/csg/CsgRayEventBatch.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayEventQuery.h"
TEST(CsgRayEventQueryContract, GroupsExactTiesWithoutMergingNearbyEvents) {
    SimpleBody same, shifted;
    same.setGeometry(new Sphere(1)); shifted.setGeometry(new Sphere(1));
    shifted.setPosition(Vector3Dd(0.000001,0,0));
    CsgBodyOperand a(same), alias(same), b(shifted);
    CsgTree root(CsgOperation::UNION);
    ASSERT_TRUE(root.addOperand(a)); ASSERT_TRUE(root.addOperand(alias)); ASSERT_TRUE(root.addOperand(b));
    CsgRayLeafBatch leaves;
    ASSERT_EQ(RaySolidQueryStatus::Supported,CsgRayLeafQuery::query(root,
        Ray::fromAffine(Vector3Dd(-3,0,0),Vector3Dd(1,0,0)),RayCrossingQuery(0,10),leaves));
    CsgRayEventBatch groups;
    ASSERT_EQ(RaySolidQueryStatus::Supported,CsgRayEventQuery::group(leaves,groups));
    ASSERT_EQ(4,groups.size());
    EXPECT_DOUBLE_EQ(2,groups.get(0)->getT()); EXPECT_DOUBLE_EQ(4,groups.get(2)->getT());
    ASSERT_EQ(2,groups.get(0)->size()); ASSERT_EQ(2,groups.get(2)->size());
    EXPECT_EQ(0,groups.get(0)->get(0)->leafIndex); EXPECT_EQ(0,groups.get(0)->get(0)->boundaryIndex);
    EXPECT_EQ(1,groups.get(0)->get(1)->leafIndex); EXPECT_EQ(0,groups.get(0)->get(1)->boundaryIndex);
    EXPECT_EQ(2,groups.get(1)->get(0)->leafIndex);
    EXPECT_GT(groups.get(1)->getT(),groups.get(0)->getT());
    EXPECT_GT(groups.get(3)->getT(),groups.get(2)->getT());
    leaves.get(2)->getClassification().insideSegments.clear();
    EXPECT_EQ(RaySolidQueryStatus::Indeterminate,CsgRayEventQuery::group(leaves,groups));
    EXPECT_EQ(0,groups.size());
}
TEST(CsgRayEventQueryContract, EmptyLeafBoundariesProduceEmptyEventBatch) {
    SimpleBody solid; solid.setGeometry(new Sphere(1));
    CsgBodyOperand operand(solid); CsgTree root(CsgOperation::UNION);
    ASSERT_TRUE(root.addOperand(operand));
    CsgRayLeafBatch leaves; CsgRayEventBatch groups;
    ASSERT_EQ(RaySolidQueryStatus::Supported,CsgRayLeafQuery::query(root,
        Ray::fromAffine(Vector3Dd(0,0,0),Vector3Dd(1,0,0)),RayCrossingQuery(0,0.5),leaves));
    ASSERT_EQ(1,leaves.size()); EXPECT_TRUE(leaves.get(0)->getClassification().insideSegments[0]);
    ASSERT_EQ(RaySolidQueryStatus::Supported,CsgRayEventQuery::group(leaves,groups));
    EXPECT_EQ(0,groups.size());
}

TEST(CsgPointProofContract, CarriesAllLeafFactsThroughPlacedNestedExpression) {
    SimpleBody left, right;
    left.setGeometry(new Sphere(1)); right.setGeometry(new Sphere(1)); right.setPosition(Vector3Dd(10,0,0));
    CsgBodyOperand a(left), b(right);
    CsgTree child(CsgOperation::UNION);
    ASSERT_TRUE(child.addOperand(a)); ASSERT_TRUE(child.addOperand(b));
    CsgPlacedExpression placed(child);
    const Matrix4x4d forward=Matrix4x4d().translation(3,0,0).multiply(Matrix4x4d().scale(2,1,1));
    const Matrix4x4d inverse=Matrix4x4d().scale(0.5,1,1).multiply(Matrix4x4d().translation(-3,0,0));
    ASSERT_EQ(AffinePlacementStatus::Valid,placed.setAffinePlacement(forward,inverse));
    CsgTree root(CsgOperation::UNION); ASSERT_TRUE(root.addOperand(placed));
    CsgPointProof proof;
    EXPECT_EQ(GeometryContainment::Outside,CsgPointClassification::classifyWithProof(root,Vector3Dd(13,0,0),0.1,proof));
    ASSERT_EQ(2,proof.getCount());
    EXPECT_EQ(&placed,proof.getEvidence(0)->getPath().getOperand(0));
    EXPECT_EQ(&a,proof.getEvidence(0)->getPath().getOperand(1));
    EXPECT_EQ(&placed,proof.getEvidence(1)->getPath().getOperand(0));
    EXPECT_EQ(&b,proof.getEvidence(1)->getPath().getOperand(1));
    EXPECT_NEAR(5,proof.getEvidence(0)->getPoint().x(),1e-12);
    EXPECT_GT(proof.getEvidence(0)->getTolerance(),0.05);
}

#include "vsdk/toolkit/environment/scene/csg/CsgExactArithmetic.h"
TEST(CsgExactArithmeticContract, ComparesDyadicProductsAndAffineEndpointsExactly) {
    EXPECT_TRUE(CsgExactArithmetic::equalProducts(0.5,6,3,1));
    EXPECT_FALSE(CsgExactArithmetic::equalProducts(0.1,0.1,0.01,1));
    EXPECT_TRUE(CsgExactArithmetic::equalProducts(java::Double::MIN_VALUE,2,java::Math::scalb(java::Double::MIN_VALUE,1),1));
    EXPECT_FALSE(CsgExactArithmetic::equalProducts(java::Double::MIN_VALUE,0.5,0,1));
    EXPECT_TRUE(CsgExactArithmetic::equalProducts(java::Double::MAX_VALUE,2,2,java::Double::MAX_VALUE));
    EXPECT_FALSE(CsgExactArithmetic::equalProducts(java::Double::MAX_VALUE,2,java::Double::MAX_VALUE,1));
    EXPECT_TRUE(CsgExactArithmetic::equalAffine(2,3,-5,1,1,0));
    EXPECT_FALSE(CsgExactArithmetic::equalAffine(1,java::Double::MAX_VALUE,1,1,java::Double::MAX_VALUE,0));
    EXPECT_FALSE(CsgExactArithmetic::equalProducts(java::Double::NaN,1,1,1));
}

TEST(CsgSolidEquivalenceContract, SpheresSupportPermutationsReflectionsAndRadiusScale) {
    SimpleBody a,b,nearby;
    a.setGeometry(new Sphere(2)); b.setGeometry(new Sphere(1)); nearby.setGeometry(new Sphere(1));
    const Matrix4x4d rotation=Matrix4x4d::identityMatrix().withVal(0,0,0).withVal(0,1,-1).withVal(1,0,1).withVal(1,1,0);
    const Matrix4x4d placement=Matrix4x4d().translation(3,0,0).multiply(rotation).multiply(Matrix4x4d().scale(2,2,2));
    const Matrix4x4d inverse=Matrix4x4d().scale(0.5,0.5,0.5).multiply(rotation.transpose()).multiply(Matrix4x4d().translation(-3,0,0));
    a.setPosition(Vector3Dd(3,0,0));
    ASSERT_EQ(AffinePlacementStatus::Valid,b.setAffinePlacement(placement,inverse));
    nearby.setPosition(Vector3Dd(3.00000001,0,0)); nearby.setScale(Vector3Dd(2,2,2));
    CsgBodyOperand aa(a),bb(b),cc(nearby);
    EXPECT_TRUE(aa.isSameSolid(bb)); EXPECT_TRUE(bb.isSameSolid(aa)); EXPECT_FALSE(aa.isSameSolid(cc));
    CsgTree difference(CsgOperation::DIFFERENCE); ASSERT_TRUE(difference.addOperand(aa)); ASSERT_TRUE(difference.addOperand(bb));
    EXPECT_EQ(GeometryContainment::Outside,CsgPointClassification::classify(difference,Vector3Dd(5,0,0),0.1));
}

TEST(CsgSolidEquivalenceContract, BoxesSupportTranslatedBoundsAndAffineEdgeReparameterization) {
    SimpleBody a,b,c,shearA,shearB;
    a.setGeometry(new Box(Vector3Dd(-1,-1,-1),Vector3Dd(1,1,1)));
    b.setGeometry(new Box(Vector3Dd(0,0,0),Vector3Dd(4,4,4)));
    b.setPosition(Vector3Dd(-1,-1,-1)); b.setScale(Vector3Dd(0.5,0.5,0.5));
    c.setGeometry(new Box(Vector3Dd(0,0,0),Vector3Dd(4,4,4)));
    c.setPosition(Vector3Dd(-1+1e-12,-1,-1)); c.setScale(Vector3Dd(0.5,0.5,0.5));
    CsgBodyOperand aa(a),bb(b),cc(c);
    EXPECT_TRUE(aa.isSameSolid(bb)); EXPECT_TRUE(bb.isSameSolid(aa)); EXPECT_FALSE(aa.isSameSolid(cc));
    shearA.setGeometry(new Box(Vector3Dd(-1,-2,-3),Vector3Dd(1,2,3)));
    shearB.setGeometry(new Box(Vector3Dd(-0.5,-2,-3),Vector3Dd(0.5,2,3)));
    const Matrix4x4d shear=Matrix4x4d::identityMatrix().withVal(0,1,0.5);
    const Matrix4x4d unshear=Matrix4x4d::identityMatrix().withVal(0,1,-0.5);
    ASSERT_EQ(AffinePlacementStatus::Valid,shearA.setAffinePlacement(shear,unshear));
    ASSERT_EQ(AffinePlacementStatus::Valid,shearB.setAffinePlacement(shear.multiply(Matrix4x4d().scale(2,1,1)),
        Matrix4x4d().scale(0.5,1,1).multiply(unshear)));
    CsgBodyOperand sa(shearA),sb(shearB); EXPECT_TRUE(sa.isSameSolid(sb)); EXPECT_TRUE(sb.isSameSolid(sa));
}

TEST(CsgSolidEquivalenceContract, HalfSpacesAndQuadricsPreserveSignAndAxisMappings) {
    SimpleBody ha,hb,hc,qa,qb,qc;
    ha.setGeometry(new PlanarHalfSpace(1,2,0,-3));
    hb.setGeometry(new PlanarHalfSpace(2,-1,0,-3));
    hc.setGeometry(new PlanarHalfSpace(-1,-2,0,3));
    const Matrix4x4d rotation=Matrix4x4d::identityMatrix().withVal(0,0,0).withVal(0,1,-1).withVal(1,0,1).withVal(1,1,0);
    ASSERT_EQ(AffinePlacementStatus::Valid,hb.setAffinePlacement(rotation,rotation.transpose()));
    CsgBodyOperand a(ha),b(hb),c(hc); EXPECT_TRUE(a.isSameSolid(b)); EXPECT_TRUE(b.isSameSolid(a)); EXPECT_FALSE(a.isSameSolid(c));
    qa.setGeometry(new QuadricSurface(Vector3Dd(1,2,3),Vector3Dd(),Vector3Dd(),-1));
    qb.setGeometry(new QuadricSurface(Vector3Dd(6,3,9),Vector3Dd(),Vector3Dd(),-3));
    qc.setGeometry(new QuadricSurface(Vector3Dd(-1,-2,-3),Vector3Dd(),Vector3Dd(),1));
    ASSERT_EQ(AffinePlacementStatus::Valid,qb.setAffinePlacement(rotation,rotation.transpose()));
    CsgBodyOperand x(qa),y(qb),z(qc); EXPECT_TRUE(x.isSameSolid(y)); EXPECT_TRUE(y.isSameSolid(x)); EXPECT_FALSE(x.isSameSolid(z));
}

TEST(CsgSolidEquivalenceContract, PlacedViewsOfSameExpressionMustHaveSamePlacement) {
    SimpleBody a; a.setGeometry(new Sphere(1)); CsgBodyOperand leaf(a);
    CsgTree child(CsgOperation::UNION); ASSERT_TRUE(child.addOperand(leaf));
    CsgPlacedExpression first(child), second(child), shifted(child);
    ASSERT_EQ(AffinePlacementStatus::Valid,first.setAffinePlacement(Matrix4x4d().translation(3,0,0),Matrix4x4d().translation(-3,0,0)));
    ASSERT_EQ(AffinePlacementStatus::Valid,second.setAffinePlacement(Matrix4x4d().translation(3,0,0),Matrix4x4d().translation(-3,0,0)));
    ASSERT_EQ(AffinePlacementStatus::Valid,shifted.setAffinePlacement(Matrix4x4d().translation(4,0,0),Matrix4x4d().translation(-4,0,0)));
    EXPECT_TRUE(first.isSameSolid(second)); EXPECT_TRUE(second.isSameSolid(first)); EXPECT_FALSE(first.isSameSolid(shifted));
    CsgTree difference(CsgOperation::DIFFERENCE); ASSERT_TRUE(difference.addOperand(first)); ASSERT_TRUE(difference.addOperand(second));
    EXPECT_EQ(GeometryContainment::Outside,CsgPointClassification::classify(difference,Vector3Dd(4,0,0),0.1));
}

TEST(CsgExactArithmeticContract, IndependentExactRationalFixtures) {
#include "csg_exact_reference.inc"
}

TEST(CsgSolidEquivalenceContract, RecognizesEquivalentCrossFamilySolids) {
    SimpleBody sphere, sphericalQuadric, complement, halfspace, linearQuadric, wrongHalfspace;
    sphere.setGeometry(new Sphere(2));
    sphericalQuadric.setGeometry(new QuadricSurface(Vector3Dd(3,3,3),Vector3Dd(),Vector3Dd(),-12));
    complement.setGeometry(new QuadricSurface(Vector3Dd(-3,-3,-3),Vector3Dd(),Vector3Dd(),12));
    halfspace.setGeometry(new PlanarHalfSpace(1,2,0,-3));
    linearQuadric.setGeometry(new QuadricSurface(Vector3Dd(),Vector3Dd(),Vector3Dd(2,4,0),-6));
    wrongHalfspace.setGeometry(new QuadricSurface(Vector3Dd(),Vector3Dd(),Vector3Dd(-2,-4,0),6));
    CsgBodyOperand a(sphere),b(sphericalQuadric),c(complement),d(halfspace),e(linearQuadric),f(wrongHalfspace);
    EXPECT_TRUE(a.isSameSolid(b)); EXPECT_TRUE(b.isSameSolid(a)); EXPECT_FALSE(a.isSameSolid(c));
    EXPECT_TRUE(d.isSameSolid(e)); EXPECT_TRUE(e.isSameSolid(d)); EXPECT_FALSE(d.isSameSolid(f));
}

TEST(CsgSolidEquivalenceContract, DoesNotEquateRoundedCoincidentBoxEndpoints) {
    SimpleBody a,b;
    a.setGeometry(new Box(Vector3Dd(0,0,0),Vector3Dd(1,1,1)));
    b.setGeometry(new Box(Vector3Dd(0,0,0),Vector3Dd(0.5,1,1)));
    const double translation=java::Math::scalb(1.0,53);
    a.setPosition(Vector3Dd(translation,0,0)); b.setPosition(Vector3Dd(translation,0,0));
    EXPECT_DOUBLE_EQ(translation+1,translation+0.5); // Binary64 endpoints round together.
    CsgBodyOperand aa(a),bb(b);
    EXPECT_FALSE(aa.isSameSolid(bb)); EXPECT_FALSE(bb.isSameSolid(aa));
}

TEST(CsgSolidEquivalenceContract, AxisHalfSpacesAllowDifferentTranslationsAndScales) {
    SimpleBody a,b,c,d;
    a.setGeometry(new PlanarHalfSpace(1,0,0,-2));
    b.setGeometry(new PlanarHalfSpace(2,0,0,-2));
    c.setGeometry(new PlanarHalfSpace(-2,0,0,2));
    d.setGeometry(new QuadricSurface(Vector3Dd(),Vector3Dd(),Vector3Dd(2,0,0),-2));
    b.setPosition(Vector3Dd(1,9,-4)); c.setPosition(Vector3Dd(1,9,-4));
    d.setPosition(Vector3Dd(1,-3,4));
    CsgBodyOperand aa(a),bb(b),cc(c),dd(d);
    EXPECT_TRUE(aa.isSameSolid(bb)); EXPECT_TRUE(bb.isSameSolid(aa));
    EXPECT_TRUE(aa.isSameSolid(dd)); EXPECT_TRUE(dd.isSameSolid(aa));
    EXPECT_FALSE(aa.isSameSolid(cc));
    b.setPosition(Vector3Dd(1+1e-12,9,-4)); EXPECT_FALSE(aa.isSameSolid(bb));
}

TEST(CsgSolidEquivalenceContract, QuadricMixedTermsAndLinearSignsTransformExactly) {
    SimpleBody a,b,c;
    a.setGeometry(new QuadricSurface(Vector3Dd(1,2,3),Vector3Dd(4,5,6),Vector3Dd(7,8,9),-10));
    b.setGeometry(new QuadricSurface(Vector3Dd(6,3,9),Vector3Dd(-12,18,-15),Vector3Dd(24,-21,27),-30));
    c.setGeometry(new QuadricSurface(Vector3Dd(6,3,9),Vector3Dd(12,18,-15),Vector3Dd(24,-21,27),-30));
    const Matrix4x4d rotation=Matrix4x4d::identityMatrix().withVal(0,0,0).withVal(0,1,-1).withVal(1,0,1).withVal(1,1,0);
    ASSERT_EQ(AffinePlacementStatus::Valid,b.setAffinePlacement(rotation,rotation.transpose()));
    ASSERT_EQ(AffinePlacementStatus::Valid,c.setAffinePlacement(rotation,rotation.transpose()));
    CsgBodyOperand aa(a),bb(b),cc(c);
    EXPECT_TRUE(aa.isSameSolid(bb)); EXPECT_TRUE(bb.isSameSolid(aa)); EXPECT_FALSE(aa.isSameSolid(cc));
}

#include "vsdk/toolkit/environment/scene/csg/CsgPointAttribution.h"
TEST(CsgPointAttributionContract, ReportsAllCoincidentOwnersAndDifferenceOrientation) {
    SimpleBody outer, inner, equalInner;
    outer.setGeometry(new Sphere(2)); inner.setGeometry(new Sphere(1)); equalInner.setGeometry(new Sphere(1));
    CsgBodyOperand a(outer), b(inner), c(equalInner);
    CsgTree united(CsgOperation::UNION), difference(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(united.addOperand(b)); ASSERT_TRUE(united.addOperand(c));
    ASSERT_TRUE(difference.addOperand(a)); ASSERT_TRUE(difference.addOperand(b)); ASSERT_TRUE(difference.addOperand(c));
    CsgPointAttribution owners;
    ASSERT_EQ(GeometryContainment::Boundary,CsgPointClassification::classifyWithAttribution(united,Vector3Dd(1,0,0),0.01,owners));
    ASSERT_TRUE(owners.isComplete()); ASSERT_EQ(2,owners.getCount());
    EXPECT_EQ(&b,owners.getOwner(0)->getOperand(0)); EXPECT_EQ(&c,owners.getOwner(1)->getOperand(0));
    EXPECT_FALSE(owners.getOwner(0)->isReversed()); EXPECT_FALSE(owners.getOwner(1)->isReversed());
    ASSERT_EQ(GeometryContainment::Boundary,CsgPointClassification::classifyWithAttribution(difference,Vector3Dd(1,0,0),0.01,owners));
    ASSERT_EQ(2,owners.getCount());
    EXPECT_TRUE(owners.getOwner(0)->isReversed()); EXPECT_TRUE(owners.getOwner(1)->isReversed());
    EXPECT_EQ(nullptr,owners.getOwner(2));
}
TEST(CsgPointAttributionContract, ExcludesHiddenCancellationAndRejectsAmbiguity) {
    SimpleBody hidden,visible,nearby;
    hidden.setGeometry(new Sphere(1)); visible.setGeometry(new Sphere(1)); nearby.setGeometry(new Sphere(1));
    nearby.setPosition(Vector3Dd(0.0001,0,0));
    CsgBodyOperand a(hidden), b(visible), c(nearby);
    CsgTree cancelled(CsgOperation::DIFFERENCE); ASSERT_TRUE(cancelled.addOperand(a)); ASSERT_TRUE(cancelled.addOperand(a));
    CsgTree root(CsgOperation::UNION); ASSERT_TRUE(root.addOperand(cancelled)); ASSERT_TRUE(root.addOperand(b));
    CsgPointAttribution owners;
    ASSERT_EQ(GeometryContainment::Boundary,CsgPointClassification::classifyWithAttribution(root,Vector3Dd(1,0,0),0.01,owners));
    ASSERT_EQ(1,owners.getCount()); EXPECT_EQ(&b,owners.getOwner(0)->getOperand(0));
    CsgTree ambiguous(CsgOperation::UNION); ASSERT_TRUE(ambiguous.addOperand(b)); ASSERT_TRUE(ambiguous.addOperand(c));
    EXPECT_EQ(GeometryContainment::Indeterminate,CsgPointClassification::classifyWithAttribution(ambiguous,Vector3Dd(1,0,0),0.01,owners));
    EXPECT_FALSE(owners.isComplete()); EXPECT_EQ(0,owners.getCount());
}

TEST(CsgSolidEquivalenceContract, EllipsoidsSupportExactNonPermutationReparameterization) {
    SimpleBody a,b,quadric,nearby;
    a.setGeometry(new Sphere(1)); b.setGeometry(new Sphere(1)); nearby.setGeometry(new Sphere(1));
    quadric.setGeometry(new QuadricSurface(Vector3Dd(1,1,1),Vector3Dd(),Vector3Dd(),-1));
    const Matrix4x4d forwardA=Matrix4x4d().scale(5,5,5), inverseA=Matrix4x4d().scale(0.2,0.2,0.2);
    const Matrix4x4d forwardB=Matrix4x4d::identityMatrix().withVal(0,0,3).withVal(0,1,-4)
        .withVal(1,0,4).withVal(1,1,3).withVal(2,2,5);
    const Matrix4x4d inverseB=Matrix4x4d::identityMatrix().withVal(0,0,0.12).withVal(0,1,0.16)
        .withVal(1,0,-0.16).withVal(1,1,0.12).withVal(2,2,0.2);
    ASSERT_EQ(AffinePlacementStatus::Valid,a.setAffinePlacement(forwardA,inverseA));
    ASSERT_EQ(AffinePlacementStatus::Valid,b.setAffinePlacement(forwardB,inverseB));
    ASSERT_EQ(AffinePlacementStatus::Valid,quadric.setAffinePlacement(forwardB,inverseB));
    nearby.setScale(Vector3Dd(5,5,4.999999));
    CsgBodyOperand aa(a),bb(b),qq(quadric),cc(nearby);
    EXPECT_TRUE(aa.isSameSolid(bb)); EXPECT_TRUE(bb.isSameSolid(aa));
    EXPECT_TRUE(aa.isSameSolid(qq)); EXPECT_TRUE(qq.isSameSolid(aa)); EXPECT_FALSE(aa.isSameSolid(cc));
}

TEST(CsgSolidEquivalenceContract, AffineBoxesSupportTranslatedLocalOrigins) {
    SimpleBody a,b,nearby;
    a.setGeometry(new Box(Vector3Dd(-1,-1,-1),Vector3Dd(1,1,1)));
    b.setGeometry(new Box(Vector3Dd(0,0,0),Vector3Dd(2,2,2)));
    nearby.setGeometry(new Box(Vector3Dd(0,0,0),Vector3Dd(2,2,2)));
    const Matrix4x4d shear=Matrix4x4d::identityMatrix().withVal(0,1,0.5);
    const Matrix4x4d unshear=Matrix4x4d::identityMatrix().withVal(0,1,-0.5);
    const Matrix4x4d shifted=Matrix4x4d().translation(-1.5,-1,-1).multiply(shear);
    const Matrix4x4d shiftedInverse=unshear.multiply(Matrix4x4d().translation(1.5,1,1));
    ASSERT_EQ(AffinePlacementStatus::Valid,a.setAffinePlacement(shear,unshear));
    ASSERT_EQ(AffinePlacementStatus::Valid,b.setAffinePlacement(shifted,shiftedInverse));
    ASSERT_EQ(AffinePlacementStatus::Valid,nearby.setAffinePlacement(
        Matrix4x4d().translation(-1.500001,-1,-1).multiply(shear),
        unshear.multiply(Matrix4x4d().translation(1.500001,1,1))));
    CsgBodyOperand aa(a),bb(b),cc(nearby);
    EXPECT_TRUE(aa.isSameSolid(bb)); EXPECT_TRUE(bb.isSameSolid(aa)); EXPECT_FALSE(aa.isSameSolid(cc));
}

TEST(CsgSolidEquivalenceContract, ObliquePlanesSupportTranslatedReflectedAndShearedFrames) {
    SimpleBody a,b,quadric,opposite;
    a.setGeometry(new PlanarHalfSpace(1,2,0,-3));
    b.setGeometry(new PlanarHalfSpace(-1,2.5,0,-2));
    quadric.setGeometry(new QuadricSurface(Vector3Dd(),Vector3Dd(),Vector3Dd(-2,5,0),-4));
    opposite.setGeometry(new PlanarHalfSpace(1,-2.5,0,2));
    const Matrix4x4d linear=Matrix4x4d::identityMatrix().withVal(0,0,-1).withVal(0,1,0.5);
    const Matrix4x4d forward=Matrix4x4d().translation(1,0,0).multiply(linear);
    const Matrix4x4d inverse=linear.multiply(Matrix4x4d().translation(-1,0,0));
    ASSERT_EQ(AffinePlacementStatus::Valid,b.setAffinePlacement(forward,inverse));
    ASSERT_EQ(AffinePlacementStatus::Valid,quadric.setAffinePlacement(forward,inverse));
    ASSERT_EQ(AffinePlacementStatus::Valid,opposite.setAffinePlacement(forward,inverse));
    CsgBodyOperand aa(a),bb(b),qq(quadric),cc(opposite);
    EXPECT_TRUE(aa.isSameSolid(bb)); EXPECT_TRUE(bb.isSameSolid(aa));
    EXPECT_TRUE(aa.isSameSolid(qq)); EXPECT_TRUE(qq.isSameSolid(aa)); EXPECT_FALSE(aa.isSameSolid(cc));
}

TEST(CsgSolidEquivalenceContract, QuadricsAllowGeneralExactAffineReparameterization) {
    SimpleBody a,b,c,sphere,shiftedQuadric;
    a.setGeometry(new QuadricSurface(Vector3Dd(1,2,3),Vector3Dd(4,5,6),Vector3Dd(7,8,9),-10));
    // F(-u_x + 0.5 u_y + 1,u_y,u_z) in the reflected/sheared frame.
    b.setGeometry(new QuadricSurface(Vector3Dd(1,4.25,3),Vector3Dd(-5,-5,8.5),Vector3Dd(-9,16.5,14),-2));
    c.setGeometry(new QuadricSurface(Vector3Dd(1,4.25,3),Vector3Dd(-5,-5,8.500001),Vector3Dd(-9,16.5,14),-2));
    const Matrix4x4d linear=Matrix4x4d::identityMatrix().withVal(0,0,-1).withVal(0,1,0.5);
    const Matrix4x4d forward=Matrix4x4d().translation(1,0,0).multiply(linear);
    const Matrix4x4d inverse=linear.multiply(Matrix4x4d().translation(-1,0,0));
    ASSERT_EQ(AffinePlacementStatus::Valid,b.setAffinePlacement(forward,inverse));
    ASSERT_EQ(AffinePlacementStatus::Valid,c.setAffinePlacement(forward,inverse));
    CsgBodyOperand aa(a),bb(b),cc(c);
    EXPECT_TRUE(aa.isSameSolid(bb)); EXPECT_TRUE(bb.isSameSolid(aa)); EXPECT_FALSE(aa.isSameSolid(cc));
    sphere.setGeometry(new Sphere(1)); sphere.setPosition(Vector3Dd(1,0,0));
    shiftedQuadric.setGeometry(new QuadricSurface(Vector3Dd(1,1,1),Vector3Dd(),Vector3Dd(-2,0,0),0));
    CsgBodyOperand ss(sphere),qq(shiftedQuadric);
    EXPECT_TRUE(ss.isSameSolid(qq)); EXPECT_TRUE(qq.isSameSolid(ss));
}

#include "vsdk/toolkit/environment/scene/csg/CsgExactQuadric.h"
TEST(CsgExactQuadricContract, IndependentRationalHomogeneousTransforms) {
#include "csg_quadric_reference.inc"
}
TEST(CsgExactQuadricContract, RejectsUnrepresentableIntermediateAtomically) {
    const double local[10]={1,1,1,java::Double::MIN_VALUE,0,0,0,0,0,-1};
    double result[10]={11,12,13,14,15,16,17,18,19,20};
    EXPECT_FALSE(CsgExactQuadric::world(local,Matrix4x4d::identityMatrix(),result));
    for (int i=0;i<10;++i) EXPECT_DOUBLE_EQ(11+i,result[i]);
}
TEST(CsgPointAttributionContract, RetainsPlacedNestedDifferenceOwners) {
    SimpleBody outer, inner, duplicate;
    outer.setGeometry(new Sphere(2)); inner.setGeometry(new Sphere(1)); duplicate.setGeometry(new Sphere(1));
    CsgBodyOperand a(outer),b(inner),c(duplicate);
    CsgTree child(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(child.addOperand(a)); ASSERT_TRUE(child.addOperand(b)); ASSERT_TRUE(child.addOperand(c));
    CsgPlacedExpression placed(child);
    ASSERT_EQ(AffinePlacementStatus::Valid,placed.setAffinePlacement(
        Matrix4x4d().translation(3,0,0).multiply(Matrix4x4d().scale(2,1,1)),
        Matrix4x4d().scale(0.5,1,1).multiply(Matrix4x4d().translation(-3,0,0))));
    CsgTree root(CsgOperation::UNION); ASSERT_TRUE(root.addOperand(placed));
    CsgPointAttribution owners;
    ASSERT_EQ(GeometryContainment::Boundary,CsgPointClassification::classifyWithAttribution(root,Vector3Dd(5,0,0),0.1,owners));
    ASSERT_EQ(2,owners.getCount());
    EXPECT_EQ(&placed,owners.getOwner(0)->getOperand(0)); EXPECT_EQ(&b,owners.getOwner(0)->getOperand(1));
    EXPECT_EQ(&placed,owners.getOwner(1)->getOperand(0)); EXPECT_EQ(&c,owners.getOwner(1)->getOperand(1));
    EXPECT_TRUE(owners.getOwner(0)->isReversed()); EXPECT_TRUE(owners.getOwner(1)->isReversed());
}

TEST(CsgPointProofContract, EquivalentUnsupportedLeafUsesCertifiedSiblingInEitherOrder) {
    SimpleBody sphere; UncertifiedTestBody quadric;
    sphere.setGeometry(new Sphere(1));
    quadric.setGeometry(new QuadricSurface(Vector3Dd(1,1,1),Vector3Dd(),Vector3Dd(),-1));
    CsgBodyOperand a(sphere),b(quadric);
    ASSERT_TRUE(a.isSameSolid(b)); ASSERT_EQ(GeometryContainment::Unsupported,b.classifyPointCertified(Vector3Dd(1,0,0),0.01));
    CsgTree first(CsgOperation::UNION), second(CsgOperation::UNION), difference(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(first.addOperand(a)); ASSERT_TRUE(first.addOperand(b));
    ASSERT_TRUE(second.addOperand(b)); ASSERT_TRUE(second.addOperand(a));
    ASSERT_TRUE(difference.addOperand(b)); ASSERT_TRUE(difference.addOperand(a));
    CsgPointAttribution owners;
    EXPECT_EQ(GeometryContainment::Boundary,CsgPointClassification::classifyWithAttribution(first,Vector3Dd(1,0,0),0.01,owners));
    ASSERT_EQ(2,owners.getCount()); EXPECT_EQ(&a,owners.getOwner(0)->getOperand(0)); EXPECT_EQ(&b,owners.getOwner(1)->getOperand(0));
    EXPECT_EQ(GeometryContainment::Boundary,CsgPointClassification::classifyWithAttribution(second,Vector3Dd(1,0,0),0.01,owners));
    ASSERT_EQ(2,owners.getCount()); EXPECT_EQ(&b,owners.getOwner(0)->getOperand(0)); EXPECT_EQ(&a,owners.getOwner(1)->getOperand(0));
    CsgPointProof proof;
    EXPECT_EQ(GeometryContainment::Boundary,CsgPointClassification::classifyWithProof(second,Vector3Dd(1,0,0),0.01,proof));
    bool found=false;
    for (long i=0;i<proof.getCount();++i) if (proof.getEvidence(i)->isEquivalence()) {
        found=true;
        EXPECT_EQ(&a,proof.getEvidence(i)->getPath().getOperand(0));
        EXPECT_EQ(&b,proof.getEvidence(i)->getEquivalentPath().getOperand(0));
    }
    EXPECT_TRUE(found);
    EXPECT_EQ(GeometryContainment::Outside,CsgPointClassification::classify(difference,Vector3Dd(1,0,0),0.01));
}
TEST(CsgPointProofContract, DoesNotTransferAcrossDistinctOrUncertainShapes) {
    SimpleBody sphere; UncertifiedTestBody quadric, opposite;
    sphere.setGeometry(new Sphere(1));
    quadric.setGeometry(new QuadricSurface(Vector3Dd(1,1,1),Vector3Dd(),Vector3Dd(),-1.000001));
    opposite.setGeometry(new QuadricSurface(Vector3Dd(-1,-1,-1),Vector3Dd(),Vector3Dd(),1));
    CsgBodyOperand a(sphere),b(quadric),c(opposite);
    CsgTree first(CsgOperation::UNION), second(CsgOperation::INTERSECTION);
    ASSERT_TRUE(first.addOperand(a)); ASSERT_TRUE(first.addOperand(b));
    ASSERT_TRUE(second.addOperand(a)); ASSERT_TRUE(second.addOperand(c));
    EXPECT_EQ(GeometryContainment::Unsupported,CsgPointClassification::classify(first,Vector3Dd(1,0,0),0.01));
    EXPECT_EQ(GeometryContainment::Unsupported,CsgPointClassification::classify(second,Vector3Dd(1,0,0),0.01));
    CsgTree equal(CsgOperation::UNION);
    UncertifiedTestBody same; same.setGeometry(new QuadricSurface(Vector3Dd(1,1,1),Vector3Dd(),Vector3Dd(),-1));
    CsgBodyOperand sameOperand(same); ASSERT_TRUE(equal.addOperand(a)); ASSERT_TRUE(equal.addOperand(sameOperand));
    EXPECT_EQ(GeometryContainment::Indeterminate,CsgPointClassification::classify(equal,Vector3Dd(1,0,0),0));
}

TEST(CsgPointProofContract, EquivalentLeafRelationKeepsNestedPlacementAndDifferenceParity) {
    SimpleBody outer, sphere; UncertifiedTestBody quadric;
    outer.setGeometry(new Sphere(2)); sphere.setGeometry(new Sphere(1));
    quadric.setGeometry(new QuadricSurface(Vector3Dd(1,1,1),Vector3Dd(),Vector3Dd(),-1));
    CsgBodyOperand a(outer),b(sphere),c(quadric);
    CsgTree child(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(child.addOperand(a)); ASSERT_TRUE(child.addOperand(b)); ASSERT_TRUE(child.addOperand(c));
    CsgPlacedExpression placed(child);
    ASSERT_EQ(AffinePlacementStatus::Valid,placed.setAffinePlacement(
        Matrix4x4d().translation(3,0,0).multiply(Matrix4x4d().scale(2,1,1)),
        Matrix4x4d().scale(0.5,1,1).multiply(Matrix4x4d().translation(-3,0,0))));
    CsgTree root(CsgOperation::UNION); ASSERT_TRUE(root.addOperand(placed));
    CsgPointProof proof;
    ASSERT_EQ(GeometryContainment::Boundary,CsgPointClassification::classifyWithProof(root,Vector3Dd(5,0,0),0.1,proof));
    bool found=false;
    for (long i=0;i<proof.getCount();++i) if (proof.getEvidence(i)->isEquivalence()) {
        found=true;
        const CsgPointWitness& source=proof.getEvidence(i)->getPath();
        const CsgPointWitness& recipient=proof.getEvidence(i)->getEquivalentPath();
        ASSERT_EQ(2,source.getLength()); ASSERT_EQ(2,recipient.getLength());
        EXPECT_EQ(&placed,source.getOperand(0)); EXPECT_EQ(&b,source.getOperand(1));
        EXPECT_EQ(&placed,recipient.getOperand(0)); EXPECT_EQ(&c,recipient.getOperand(1));
        EXPECT_TRUE(source.isReversed()); EXPECT_TRUE(recipient.isReversed());
    }
    EXPECT_TRUE(found);
    CsgPointAttribution owners;
    ASSERT_EQ(GeometryContainment::Boundary,CsgPointClassification::classifyWithAttribution(root,Vector3Dd(5,0,0),0.1,owners));
    ASSERT_EQ(2,owners.getCount()); EXPECT_TRUE(owners.getOwner(0)->isReversed()); EXPECT_TRUE(owners.getOwner(1)->isReversed());
}

TEST(CsgSolidEquivalenceContract, IndependentlyBuiltCompositionsPreserveBooleanSemantics) {
    SimpleBody first, second, third;
    first.setGeometry(new Sphere(1)); second.setGeometry(new Sphere(1)); third.setGeometry(new Sphere(1));
    second.setPosition(Vector3Dd(4,0,0)); third.setPosition(Vector3Dd(8,0,0));
    CsgBodyOperand a(first), b(second), c(third);
    CsgTree left(CsgOperation::UNION), right(CsgOperation::UNION), wrong(CsgOperation::UNION);
    ASSERT_TRUE(left.addOperand(a)); ASSERT_TRUE(left.addOperand(b));
    ASSERT_TRUE(right.addOperand(b)); ASSERT_TRUE(right.addOperand(a));
    ASSERT_TRUE(wrong.addOperand(a)); ASSERT_TRUE(wrong.addOperand(c));
    EXPECT_TRUE(left.isSameSolid(right)); EXPECT_TRUE(right.isSameSolid(left)); EXPECT_FALSE(left.isSameSolid(wrong));
    CsgTree minusA(CsgOperation::DIFFERENCE), minusB(CsgOperation::DIFFERENCE), minusWrong(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(minusA.addOperand(a)); ASSERT_TRUE(minusA.addOperand(b)); ASSERT_TRUE(minusA.addOperand(c));
    ASSERT_TRUE(minusB.addOperand(a)); ASSERT_TRUE(minusB.addOperand(c)); ASSERT_TRUE(minusB.addOperand(b));
    ASSERT_TRUE(minusWrong.addOperand(b)); ASSERT_TRUE(minusWrong.addOperand(a)); ASSERT_TRUE(minusWrong.addOperand(c));
    EXPECT_TRUE(minusA.isSameSolid(minusB)); EXPECT_FALSE(minusA.isSameSolid(minusWrong));
    CsgPlacedExpression placedA(left), placedB(right), shifted(right);
    const Matrix4x4d p=Matrix4x4d().translation(2,0,0), inv=Matrix4x4d().translation(-2,0,0);
    ASSERT_EQ(AffinePlacementStatus::Valid,placedA.setAffinePlacement(p,inv));
    ASSERT_EQ(AffinePlacementStatus::Valid,placedB.setAffinePlacement(p,inv));
    ASSERT_EQ(AffinePlacementStatus::Valid,shifted.setAffinePlacement(Matrix4x4d().translation(3,0,0),Matrix4x4d().translation(-3,0,0)));
    EXPECT_TRUE(placedA.isSameSolid(placedB)); EXPECT_FALSE(placedA.isSameSolid(shifted));
    CsgTree composed(CsgOperation::UNION); ASSERT_TRUE(composed.addOperand(left)); ASSERT_TRUE(composed.addOperand(right));
    CsgPointAttribution owners;
    EXPECT_EQ(GeometryContainment::Boundary,CsgPointClassification::classifyWithAttribution(composed,Vector3Dd(1,0,0),0.01,owners));
    ASSERT_EQ(2,owners.getCount());
    EXPECT_EQ(&left,owners.getOwner(0)->getOperand(0)); EXPECT_EQ(&a,owners.getOwner(0)->getOperand(1));
    EXPECT_EQ(&right,owners.getOwner(1)->getOperand(0)); EXPECT_EQ(&a,owners.getOwner(1)->getOperand(1));
}

#include "CsgCycleTestOperand.h"
TEST(CsgSolidEquivalenceContract, DoesNotDeclareCyclicExpressionsEquivalent) {
    CsgCycleTestOperand a,b;
    CsgPlacedExpression first(a), second(b);
    EXPECT_FALSE(first.isSameSolid(second)); EXPECT_FALSE(second.isSameSolid(first));
    CsgTree root(CsgOperation::UNION);
    ASSERT_TRUE(root.addOperand(first));
    EXPECT_EQ(GeometryContainment::InvalidInput,CsgPointClassification::classify(root,Vector3Dd(),0.1));
}

#include "CsgPolicyTestOperand.h"
TEST(CsgSolidEquivalenceContract, DoesNotIgnoreUnknownScenePolicy) {
    SimpleBody sphere; sphere.setGeometry(new Sphere(1));
    CsgBodyOperand leaf(sphere);
    CsgTree child(CsgOperation::UNION); ASSERT_TRUE(child.addOperand(leaf));
    CsgPolicyTestOperand normal(child,false), inverted(child,true);
    CsgTree first(CsgOperation::UNION), second(CsgOperation::UNION);
    ASSERT_TRUE(first.addOperand(normal)); ASSERT_TRUE(second.addOperand(inverted));
    EXPECT_FALSE(first.isSameSolid(second)); EXPECT_FALSE(second.isSameSolid(first));
    EXPECT_EQ(GeometryContainment::Unsupported,CsgPointClassification::classify(first,Vector3Dd(),0.1));
    CsgRayLeafBatch leaves;
    EXPECT_EQ(RaySolidQueryStatus::Unsupported,CsgRayLeafQuery::query(first,
        Ray::fromAffine(Vector3Dd(-3,0,0),Vector3Dd(1,0,0)),RayCrossingQuery(0,10),leaves));
    EXPECT_EQ(0,leaves.size());
}

#include "CsgPolicyTestTree.h"
TEST(CsgSolidEquivalenceContract, SceneSubclassMustOptIntoStructuralTraversal) {
    SimpleBody sphere; sphere.setGeometry(new Sphere(1)); CsgBodyOperand leaf(sphere);
    CsgPolicyTestTree custom; ASSERT_TRUE(custom.addOperand(leaf));
    CsgTree root(CsgOperation::UNION); ASSERT_TRUE(root.addOperand(custom));
    EXPECT_FALSE(custom.isStructuralTraversalSafe());
    EXPECT_EQ(GeometryContainment::Unsupported,CsgPointClassification::classify(root,Vector3Dd(),0.1));
    CsgRayLeafBatch leaves;
    EXPECT_EQ(RaySolidQueryStatus::Unsupported,CsgRayLeafQuery::query(root,
        Ray::fromAffine(Vector3Dd(-3,0,0),Vector3Dd(1,0,0)),RayCrossingQuery(0,10),leaves));
}

#include "CsgOpaqueTestExpression.h"
TEST(CsgSolidEquivalenceContract, OpaqueExpressionMustOptIntoTraversal) {
    SimpleBody sphere; sphere.setGeometry(new Sphere(1)); CsgBodyOperand leaf(sphere);
    CsgOpaqueTestExpression opaque(leaf); CsgPlacedExpression wrapped(opaque);
    CsgTree root(CsgOperation::UNION); ASSERT_TRUE(root.addOperand(wrapped));
    EXPECT_EQ(GeometryContainment::Unsupported,CsgPointClassification::classify(root,Vector3Dd(),0.1));
    CsgRayLeafBatch leaves;
    EXPECT_EQ(RaySolidQueryStatus::Unsupported,CsgRayLeafQuery::query(root,
        Ray::fromAffine(Vector3Dd(-3,0,0),Vector3Dd(1,0,0)),RayCrossingQuery(0,10),leaves));
    EXPECT_EQ(0,leaves.size());
}

TEST(CsgSolidEquivalenceContract, AssociatesAndDeduplicatesBooleanSolidSets) {
    SimpleBody one,two,three;
    one.setGeometry(new Sphere(1)); two.setGeometry(new Sphere(1)); three.setGeometry(new Sphere(1));
    two.setPosition(Vector3Dd(4,0,0)); three.setPosition(Vector3Dd(8,0,0));
    CsgBodyOperand a(one),b(two),c(three);
    CsgTree inner(CsgOperation::UNION), flat(CsgOperation::UNION), nested(CsgOperation::UNION);
    ASSERT_TRUE(inner.addOperand(c)); ASSERT_TRUE(inner.addOperand(b));
    ASSERT_TRUE(flat.addOperand(a)); ASSERT_TRUE(flat.addOperand(b)); ASSERT_TRUE(flat.addOperand(c));
    ASSERT_TRUE(nested.addOperand(inner)); ASSERT_TRUE(nested.addOperand(a)); ASSERT_TRUE(nested.addOperand(a));
    EXPECT_TRUE(flat.isSameSolid(nested)); EXPECT_TRUE(nested.isSameSolid(flat));
    CsgTree innerI(CsgOperation::INTERSECTION), flatI(CsgOperation::INTERSECTION), nestedI(CsgOperation::INTERSECTION);
    ASSERT_TRUE(innerI.addOperand(c)); ASSERT_TRUE(innerI.addOperand(b));
    ASSERT_TRUE(flatI.addOperand(a)); ASSERT_TRUE(flatI.addOperand(b)); ASSERT_TRUE(flatI.addOperand(c));
    ASSERT_TRUE(nestedI.addOperand(innerI)); ASSERT_TRUE(nestedI.addOperand(a)); ASSERT_TRUE(nestedI.addOperand(a));
    EXPECT_TRUE(flatI.isSameSolid(nestedI));
    CsgTree minusFlat(CsgOperation::DIFFERENCE), minusUnion(CsgOperation::DIFFERENCE), wrong(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(minusFlat.addOperand(a)); ASSERT_TRUE(minusFlat.addOperand(b)); ASSERT_TRUE(minusFlat.addOperand(c));
    ASSERT_TRUE(minusUnion.addOperand(a)); ASSERT_TRUE(minusUnion.addOperand(inner));
    ASSERT_TRUE(wrong.addOperand(b)); ASSERT_TRUE(wrong.addOperand(a)); ASSERT_TRUE(wrong.addOperand(c));
    EXPECT_TRUE(minusFlat.isSameSolid(minusUnion)); EXPECT_FALSE(minusFlat.isSameSolid(wrong));
    CsgPlacedExpression translated(inner);
    ASSERT_EQ(AffinePlacementStatus::Valid,translated.setAffinePlacement(Matrix4x4d().translation(1,0,0),Matrix4x4d().translation(-1,0,0)));
    CsgTree different(CsgOperation::UNION); ASSERT_TRUE(different.addOperand(a)); ASSERT_TRUE(different.addOperand(translated));
    EXPECT_FALSE(flat.isSameSolid(different));
}

TEST(CsgPointProofContract, EquivalentCompositeCanReuseProofWithoutInventingOwners) {
    SimpleBody sphere; UncertifiedTestBody quadric;
    sphere.setGeometry(new Sphere(1));
    quadric.setGeometry(new QuadricSurface(Vector3Dd(1,1,1),Vector3Dd(),Vector3Dd(),-1));
    CsgBodyOperand a(sphere),b(quadric);
    CsgTree known(CsgOperation::UNION), unknown(CsgOperation::UNION), root(CsgOperation::UNION);
    ASSERT_TRUE(known.addOperand(a)); ASSERT_TRUE(unknown.addOperand(b));
    ASSERT_TRUE(root.addOperand(unknown)); ASSERT_TRUE(root.addOperand(known));
    ASSERT_TRUE(known.isSameSolid(unknown));
    CsgPointProof proof;
    ASSERT_EQ(GeometryContainment::Boundary,CsgPointClassification::classifyWithProof(root,Vector3Dd(1,0,0),0.01,proof));
    ASSERT_TRUE(proof.isAvailable());
    bool relation=false;
    for (long i=0;i<proof.getCount();++i) if (proof.getEvidence(i)->isEquivalence()) {
        const CsgPointEvidence* fact=proof.getEvidence(i);
        if (fact->getPath().getOperand(0)==&known && fact->getEquivalentPath().getOperand(0)==&unknown) relation=true;
    }
    EXPECT_TRUE(relation);
    CsgPointAttribution owners;
    EXPECT_EQ(GeometryContainment::Boundary,CsgPointClassification::classifyWithAttribution(root,Vector3Dd(1,0,0),0.01,owners));
    EXPECT_FALSE(owners.isComplete());
    CsgPointWitness witness;
    EXPECT_EQ(GeometryContainment::Boundary,CsgPointClassification::classifyWithWitness(root,Vector3Dd(1,0,0),0.01,witness));
    ASSERT_EQ(2,witness.getLength()); EXPECT_EQ(&known,witness.getOperand(0)); EXPECT_EQ(&a,witness.getOperand(1));
}

#include "vsdk/toolkit/environment/scene/csg/CsgDyadic.h"
TEST(CsgDyadicContract, PreservesExtremeTermsCancellationAndNonRepresentableProducts) {
    CsgDyadic a,b,c,d,product,rounded,sum,recovered;
    ASSERT_TRUE(CsgDyadic::fromDouble(0.1,a)); ASSERT_TRUE(CsgDyadic::fromDouble(0.1,b));
    ASSERT_TRUE(CsgDyadic::multiply(a,b,product)); ASSERT_TRUE(CsgDyadic::fromDouble(0.01,rounded));
    EXPECT_FALSE(product.equals(rounded));
    ASSERT_TRUE(CsgDyadic::fromDouble(java::Double::MAX_VALUE,a));
    ASSERT_TRUE(CsgDyadic::fromDouble(java::Double::MIN_VALUE,b));
    ASSERT_TRUE(CsgDyadic::add(a,b,sum)); ASSERT_TRUE(CsgDyadic::subtract(sum,a,recovered)); EXPECT_TRUE(recovered.equals(b));
    ASSERT_TRUE(CsgDyadic::fromDouble(-0.125,c)); ASSERT_TRUE(CsgDyadic::fromDouble(0.75,d));
    CsgDyadic ab,ac,ad,lhs,bc,rhs;
    ASSERT_TRUE(CsgDyadic::add(b,c,bc)); ASSERT_TRUE(CsgDyadic::multiply(a,bc,lhs));
    ASSERT_TRUE(CsgDyadic::multiply(a,b,ab)); ASSERT_TRUE(CsgDyadic::multiply(a,c,ac));
    ASSERT_TRUE(CsgDyadic::add(ab,ac,rhs)); EXPECT_TRUE(lhs.equals(rhs));
    ASSERT_TRUE(CsgDyadic::multiply(c,d,ad)); ASSERT_TRUE(CsgDyadic::subtract(ad,ad,recovered)); EXPECT_EQ(0,recovered.getSign());
}
TEST(CsgSolidEquivalenceContract, DyadicCertificatesHandleUnrepresentableAffineIntermediates) {
    SimpleBody planeA,planeB,quadricA,quadricB,sphere,translatedQuadric,nearby,nearbyQuadric;
    planeA.setGeometry(new PlanarHalfSpace(0,1,0,-1));
    planeB.setGeometry(new PlanarHalfSpace(0,1,0.1,-1));
    quadricA.setGeometry(new QuadricSurface(Vector3Dd(),Vector3Dd(0,0,1),Vector3Dd(),0));
    quadricB.setGeometry(new QuadricSurface(Vector3Dd(0,0,0.1),Vector3Dd(0,0,1),Vector3Dd(),0));
    nearbyQuadric.setGeometry(new QuadricSurface(Vector3Dd(0,0,java::Math::nextAfter(0.1,1)),Vector3Dd(0,0,1),Vector3Dd(),0));
    sphere.setGeometry(new Sphere(1));
    translatedQuadric.setGeometry(new QuadricSurface(Vector3Dd(1,1,1),Vector3Dd(),Vector3Dd(0,0,2),0));
    nearby.setGeometry(new PlanarHalfSpace(0,1,java::Math::nextAfter(0.1,1),-1));
    const Matrix4x4d linear=Matrix4x4d::identityMatrix().withVal(0,1,0.1).withVal(1,2,0.1);
    const Matrix4x4d inverse=Matrix4x4d::identityMatrix().withVal(0,1,-0.1).withVal(0,2,0.01).withVal(1,2,-0.1);
    ASSERT_EQ(AffinePlacementStatus::Valid,planeB.setAffinePlacement(linear,inverse));
    ASSERT_EQ(AffinePlacementStatus::Valid,quadricB.setAffinePlacement(linear,inverse));
    ASSERT_EQ(AffinePlacementStatus::Valid,nearbyQuadric.setAffinePlacement(linear,inverse));
    ASSERT_EQ(AffinePlacementStatus::Valid,nearby.setAffinePlacement(linear,inverse));
    ASSERT_EQ(AffinePlacementStatus::Valid,sphere.setAffinePlacement(linear,inverse));
    const Matrix4x4d shifted=Matrix4x4d().translation(0,0.1,1).multiply(linear);
    const Matrix4x4d shiftedInverse=inverse.multiply(Matrix4x4d().translation(0,-0.1,-1));
    ASSERT_EQ(AffinePlacementStatus::Valid,translatedQuadric.setAffinePlacement(shifted,shiftedInverse));
    CsgBodyOperand pa(planeA),pb(planeB),qa(quadricA),qb(quadricB),s(sphere),q(translatedQuadric),n(nearby),nq(nearbyQuadric);
    EXPECT_TRUE(pa.isSameSolid(pb)); EXPECT_TRUE(pb.isSameSolid(pa)); EXPECT_FALSE(pa.isSameSolid(n));
    EXPECT_TRUE(qa.isSameSolid(qb)); EXPECT_TRUE(qb.isSameSolid(qa)); EXPECT_FALSE(qa.isSameSolid(nq));
    EXPECT_TRUE(s.isSameSolid(q)); EXPECT_TRUE(q.isSameSolid(s));
}

TEST(CsgDyadicContract, IndependentRationalMultiProductFixtures) {
#include "csg_dyadic_reference.inc"
}

TEST(CsgDyadicContract, CapacityAndRoundingFailuresAreAtomic) {
    CsgDyadic high,one,sentinel,result;
    ASSERT_TRUE(CsgDyadic::fromDouble(2,high)); ASSERT_TRUE(CsgDyadic::fromDouble(1,one));
    ASSERT_TRUE(CsgDyadic::fromDouble(17,sentinel));
    for (int i=0;i<16;++i) ASSERT_TRUE(CsgDyadic::multiply(high,high,high));
    result=sentinel;
    EXPECT_FALSE(CsgDyadic::add(high,one,result)); EXPECT_TRUE(result.equals(sentinel));
    CsgDyadic dense,square;
    ASSERT_TRUE(CsgDyadic::fromDouble(2,high));
    for (int i=0;i<14;++i) ASSERT_TRUE(CsgDyadic::multiply(high,high,high));
    ASSERT_TRUE(CsgDyadic::add(high,one,dense));
    ASSERT_TRUE(CsgDyadic::multiply(dense,dense,square));
    EXPECT_FALSE(CsgDyadic::multiply(square,square,result)); EXPECT_TRUE(result.equals(sentinel));
    const int original=fegetround();
    if (fesetround(FE_DOWNWARD)!=0) GTEST_SKIP() << "Rounding mode cannot be changed";
    const bool success=CsgDyadic::fromDouble(1.25,result);
    fesetround(original);
    EXPECT_FALSE(success); EXPECT_TRUE(result.equals(sentinel));
}

TEST(CsgSolidEquivalenceContract, DyadicGramHandlesRoundedRadiusProducts) {
    SimpleBody a,b,nearby;
    a.setGeometry(new Sphere(0.1)); b.setGeometry(new Sphere(0.1));
    nearby.setGeometry(new Sphere(java::Math::nextAfter(0.1,1)));
    const Matrix4x4d forwardA=Matrix4x4d().scale(5,5,5), inverseA=Matrix4x4d().scale(0.2,0.2,0.2);
    const Matrix4x4d forwardB=Matrix4x4d::identityMatrix().withVal(0,0,3).withVal(0,1,-4)
        .withVal(1,0,4).withVal(1,1,3).withVal(2,2,5);
    const Matrix4x4d inverseB=Matrix4x4d::identityMatrix().withVal(0,0,0.12).withVal(0,1,0.16)
        .withVal(1,0,-0.16).withVal(1,1,0.12).withVal(2,2,0.2);
    ASSERT_EQ(AffinePlacementStatus::Valid,a.setAffinePlacement(forwardA,inverseA));
    ASSERT_EQ(AffinePlacementStatus::Valid,b.setAffinePlacement(forwardB,inverseB));
    ASSERT_EQ(AffinePlacementStatus::Valid,nearby.setAffinePlacement(forwardB,inverseB));
    CsgBodyOperand aa(a),bb(b),cc(nearby);
    EXPECT_TRUE(aa.isSameSolid(bb)); EXPECT_TRUE(bb.isSameSolid(aa)); EXPECT_FALSE(aa.isSameSolid(cc));
}

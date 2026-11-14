#include <gtest/gtest.h>
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/scene/csg/CsgMembershipRule.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingQuery.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/PlanarHalfSpace.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/scene/csg/CsgBodyOperand.h"
#include "vsdk/toolkit/environment/scene/csg/CsgCrossingMembershipFilter.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPlacedExpression.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointClassification.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayClassification.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRaySurfaceEvents.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRothRayClassifier.h"
#include "vsdk/toolkit/environment/scene/csg/CsgTree.h"
#include "vsdk/toolkit/environment/scene/csg/CsgSegmentClassification.txx"
#include "vsdk/toolkit/environment/scene/csg/CsgBody.h"
#include "CsgTestOperand.h"
#include "UncertifiedTestBody.h"

static const Ray alongX = Ray::fromAffine(Vector3Dd(-3,0,0),Vector3Dd(1,0,0));

TEST(CsgRothRayClassifierContract, CombinesTableThreeOperatorsWithOwners) {
    SimpleBody left, right;
    left.setGeometry(new Sphere(1)); right.setGeometry(new Sphere(1));
    right.setPosition(Vector3Dd(1.5,0,0));
    CsgBodyOperand a(left), b(right);
    CsgTree unionTree(CsgOperation::UNION), intersection(CsgOperation::INTERSECTION),
        difference(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(unionTree.addOperand(a)); ASSERT_TRUE(unionTree.addOperand(b));
    ASSERT_TRUE(intersection.addOperand(a)); ASSERT_TRUE(intersection.addOperand(b));
    ASSERT_TRUE(difference.addOperand(a)); ASSERT_TRUE(difference.addOperand(b));
    CsgRayClassification result;

    ASSERT_EQ(RaySolidQueryStatus::Supported,
        CsgRothRayClassifier::classify(unionTree,alongX,RayCrossingQuery(0,20),result));
    const RaySolidClassification& u=result.getSolid();
    ASSERT_EQ(2,u.boundaries.size()); ASSERT_EQ(3,u.insideSegments.size());
    EXPECT_DOUBLE_EQ(2,u.boundaries[0].t); EXPECT_DOUBLE_EQ(5.5,u.boundaries[1].t);
    EXPECT_FALSE(u.insideSegments[0]); EXPECT_TRUE(u.insideSegments[1]); EXPECT_FALSE(u.insideSegments[2]);
    ASSERT_EQ(2,result.getOwnerGroupCount());
    ASSERT_EQ(1,result.getOwnerCount(0)); EXPECT_EQ(0,result.getOwner(0,0)->leafIndex);
    ASSERT_EQ(1,result.getOwnerCount(1)); EXPECT_EQ(1,result.getOwner(1,0)->leafIndex);
    EXPECT_EQ(&a,result.getLeaves().get(0)->getPath().getOperand(0));

    ASSERT_EQ(RaySolidQueryStatus::Supported,
        CsgRothRayClassifier::classify(intersection,alongX,RayCrossingQuery(0,20),result));
    ASSERT_EQ(2,result.getSolid().boundaries.size());
    EXPECT_DOUBLE_EQ(3.5,result.getSolid().boundaries[0].t);
    EXPECT_DOUBLE_EQ(4,result.getSolid().boundaries[1].t);
    EXPECT_EQ(1,result.getOwner(0,0)->leafIndex);
    EXPECT_EQ(0,result.getOwner(1,0)->leafIndex);

    ASSERT_EQ(RaySolidQueryStatus::Supported,
        CsgRothRayClassifier::classify(difference,alongX,RayCrossingQuery(0,20),result));
    const RaySolidClassification& d=result.getSolid();
    ASSERT_EQ(2,d.boundaries.size());
    EXPECT_DOUBLE_EQ(2,d.boundaries[0].t); EXPECT_DOUBLE_EQ(3.5,d.boundaries[1].t);
    EXPECT_FALSE(d.boundaries[0].normalReversed);
    EXPECT_TRUE(d.boundaries[1].normalReversed);
    EXPECT_EQ(1,result.getOwner(1,0)->leafIndex);
    EXPECT_TRUE(result.getLeaves().get(1)->getPath().isReversed());
}

TEST(CsgRothRayClassifierContract, CoincidentEventsGiveRegularizedResult) {
    SimpleBody first, second, same;
    first.setGeometry(new Box(2,2,2)); second.setGeometry(new Box(2,2,2)); same.setGeometry(new Box(2,2,2));
    second.setPosition(Vector3Dd(2,0,0));
    CsgBodyOperand a(first), b(second), c(same);
    CsgRayClassification result;

    CsgTree touching(CsgOperation::UNION);
    ASSERT_TRUE(touching.addOperand(a)); ASSERT_TRUE(touching.addOperand(b));
    ASSERT_EQ(RaySolidQueryStatus::Supported,
        CsgRothRayClassifier::classify(touching,alongX,RayCrossingQuery(0,20),result));
    ASSERT_EQ(4.0,result.getLeaves().get(0)->getClassification().boundaries[1].t);
    ASSERT_EQ(4.0,result.getLeaves().get(1)->getClassification().boundaries[0].t);
    ASSERT_EQ(2,result.getSolid().boundaries.size());
    EXPECT_DOUBLE_EQ(2,result.getSolid().boundaries[0].t);
    EXPECT_DOUBLE_EQ(6,result.getSolid().boundaries[1].t);

    CsgTree faceContact(CsgOperation::INTERSECTION);
    ASSERT_TRUE(faceContact.addOperand(a)); ASSERT_TRUE(faceContact.addOperand(b));
    ASSERT_EQ(RaySolidQueryStatus::Supported,
        CsgRothRayClassifier::classify(faceContact,alongX,RayCrossingQuery(0,20),result));
    EXPECT_EQ(0,result.getSolid().boundaries.size());
    ASSERT_EQ(1,result.getSolid().insideSegments.size());
    EXPECT_FALSE(result.getSolid().insideSegments[0]);

    CsgTree identical(CsgOperation::INTERSECTION);
    ASSERT_TRUE(identical.addOperand(a)); ASSERT_TRUE(identical.addOperand(c));
    ASSERT_EQ(RaySolidQueryStatus::Supported,
        CsgRothRayClassifier::classify(identical,alongX,RayCrossingQuery(0,20),result));
    ASSERT_EQ(2,result.getSolid().boundaries.size());
    // No single face changes the intersection; both are joint owners.
    ASSERT_EQ(2,result.getOwnerCount(0));
    ASSERT_EQ(2,result.getOwnerCount(1));

    CsgTree emptyDifference(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(emptyDifference.addOperand(a)); ASSERT_TRUE(emptyDifference.addOperand(c));
    ASSERT_EQ(RaySolidQueryStatus::Supported,
        CsgRothRayClassifier::classify(emptyDifference,alongX,RayCrossingQuery(0,20),result));
    EXPECT_EQ(0,result.getSolid().boundaries.size());
    EXPECT_FALSE(result.getSolid().insideSegments[0]);
}

TEST(CsgRothRayClassifierContract, InteriorStartUnboundedOperandAndFailures) {
    SimpleBody ball, half;
    ball.setGeometry(new Sphere(1)); half.setGeometry(new PlanarHalfSpace(1,0,0,0));
    CsgBodyOperand a(ball), h(half);
    CsgRayClassification result;
    CsgTree inside(CsgOperation::UNION);
    ASSERT_TRUE(inside.addOperand(a));
    ASSERT_EQ(RaySolidQueryStatus::Supported, CsgRothRayClassifier::classify(inside,
        Ray::fromAffine(Vector3Dd(0,0,0),Vector3Dd(1,0,0)),RayCrossingQuery(0,5),result));
    ASSERT_EQ(1,result.getSolid().boundaries.size());
    EXPECT_TRUE(result.getSolid().insideSegments[0]);
    EXPECT_DOUBLE_EQ(1,result.getSolid().boundaries[0].t);

    CsgTree cut(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(cut.addOperand(h)); ASSERT_TRUE(cut.addOperand(a));
    ASSERT_EQ(RaySolidQueryStatus::Supported,
        CsgRothRayClassifier::classify(cut,alongX,RayCrossingQuery(0,20),result));
    const bool halfStartsInside=result.getLeaves().get(0)->getClassification().insideSegments[0];
    EXPECT_EQ(halfStartsInside,result.getSolid().insideSegments[0]);
    for (long int i=0;i+1<result.getSolid().insideSegments.size();++i)
        EXPECT_NE(result.getSolid().insideSegments[i],result.getSolid().insideSegments[i+1]);

    CsgTestOperand uncertain(GeometryContainment::Indeterminate);
    CsgTree partial(CsgOperation::UNION);
    ASSERT_TRUE(partial.addOperand(a)); ASSERT_TRUE(partial.addOperand(uncertain));
    EXPECT_EQ(RaySolidQueryStatus::Unsupported,
        CsgRothRayClassifier::classify(partial,alongX,RayCrossingQuery(0,20),result));
    EXPECT_EQ(0,result.getSolid().insideSegments.size());
    EXPECT_EQ(0,result.getLeaves().size());
    EXPECT_EQ(RaySolidQueryStatus::InvalidInput,
        CsgRothRayClassifier::classify(inside,alongX,RayCrossingQuery(2,2),result));
}

TEST(CsgRothRayClassifierContract, PlacedNestedDifferenceKeepsLeafProvenance) {
    SimpleBody outer, hole;
    outer.setGeometry(new Sphere(2)); hole.setGeometry(new Sphere(1));
    CsgBodyOperand a(outer), b(hole);
    CsgTree shell(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(shell.addOperand(a)); ASSERT_TRUE(shell.addOperand(b));
    CsgPlacedExpression placed(shell);
    ASSERT_EQ(AffinePlacementStatus::Valid,placed.setAffinePlacement(
        Matrix4x4d().translation(3,0,0).multiply(Matrix4x4d().scale(2,1,1)),
        Matrix4x4d().scale(0.5,1,1).multiply(Matrix4x4d().translation(-3,0,0))));
    CsgTree root(CsgOperation::UNION);
    ASSERT_TRUE(root.addOperand(placed));
    CsgRayClassification result;
    ASSERT_EQ(RaySolidQueryStatus::Supported, CsgRothRayClassifier::classify(root,
        Ray::fromAffine(Vector3Dd(-5,0,0),Vector3Dd(1,0,0)),RayCrossingQuery(0,20),result));
    const RaySolidClassification& s=result.getSolid();
    ASSERT_EQ(4,s.boundaries.size());
    const double expected[4]={4,6,10,12};
    for (int i=0;i<4;++i) EXPECT_DOUBLE_EQ(expected[i],s.boundaries[i].t);
    EXPECT_FALSE(s.boundaries[0].normalReversed); EXPECT_TRUE(s.boundaries[1].normalReversed);
    EXPECT_TRUE(s.boundaries[2].normalReversed); EXPECT_FALSE(s.boundaries[3].normalReversed);
    const CsgRayLeaf* holeLeaf=result.getLeaves().get(result.getOwner(1,0)->leafIndex);
    ASSERT_EQ(2,holeLeaf->getPath().getLength());
    EXPECT_EQ(&placed,holeLeaf->getPath().getOperand(0));
    EXPECT_EQ(&b,holeLeaf->getPath().getOperand(1));
    ASSERT_EQ(1,holeLeaf->getPlacementCount());
}

// Independent check: each segment's membership must match the certified point
// classification of the expression at the segment midpoint.
TEST(CsgRothRayClassifierContract, SegmentsMatchPointClassificationOnRandomTrees) {
    unsigned int seed=12345;
    const int bodyCount=6;
    SimpleBody bodies[bodyCount];
    for (int i=0;i<bodyCount;++i) {
        if (i%2==0) bodies[i].setGeometry(new Sphere(0.6+0.1*i));
        else bodies[i].setGeometry(new Box(1.0+0.2*i,1.4,0.9+0.1*i));
    }
    long int checked=0;
    for (int trial=0;trial<200;++trial) {
        for (int i=0;i<bodyCount;++i) {
            seed=seed*1103515245u+12345u; const double x=((seed>>8)%2000)/1000.0-1.0;
            seed=seed*1103515245u+12345u; const double y=((seed>>8)%2000)/2000.0-0.5;
            bodies[i].setPosition(Vector3Dd(x,y,0.1*i-0.25));
        }
        CsgBodyOperand o0(bodies[0]), o1(bodies[1]), o2(bodies[2]), o3(bodies[3]), o4(bodies[4]), o5(bodies[5]);
        const CsgOperation ops[3]={CsgOperation::UNION,CsgOperation::INTERSECTION,CsgOperation::DIFFERENCE};
        CsgTree inner(ops[trial%3]), middle(ops[(trial/3)%3]), root(ops[(trial/9)%3]);
        ASSERT_TRUE(inner.addOperand(o0)); ASSERT_TRUE(inner.addOperand(o1)); ASSERT_TRUE(inner.addOperand(o2));
        ASSERT_TRUE(middle.addOperand(o3)); ASSERT_TRUE(middle.addOperand(inner));
        ASSERT_TRUE(root.addOperand(middle)); ASSERT_TRUE(root.addOperand(o4)); ASSERT_TRUE(root.addOperand(o5));
        seed=seed*1103515245u+12345u; const double oy=((seed>>8)%1000)/1000.0-0.5;
        const Ray ray=Ray::fromAffine(Vector3Dd(-4,oy,0.05),Vector3Dd(1,0.03,-0.02));
        const double minT=0, maxT=8;
        CsgRayClassification result;
        ASSERT_EQ(RaySolidQueryStatus::Supported,
            CsgRothRayClassifier::classify(root,ray,RayCrossingQuery(minT,maxT),result));
        const RaySolidClassification& s=result.getSolid();
        ASSERT_EQ(s.boundaries.size()+1,s.insideSegments.size());
        for (long int i=0;i<s.insideSegments.size();++i) {
            const double a=i==0 ? minT : s.boundaries[i-1].t;
            const double b=i==s.boundaries.size() ? maxT : s.boundaries[i].t;
            if (b-a<1e-6) continue;
            const double t=0.5*(a+b);
            const Vector3Dd p=ray.getOrigin().add(ray.getDirection().multiply(t));
            const GeometryContainment c=CsgPointClassification::classify(root,p,0);
            if (c!=GeometryContainment::Inside && c!=GeometryContainment::Outside) continue;
            EXPECT_EQ(c==GeometryContainment::Inside,s.insideSegments[i]) << "trial " << trial << " segment " << i;
            ++checked;
        }
        for (long int i=0;i<s.boundaries.size();++i) {
            EXPECT_GT(result.getOwnerCount(i),0);
        }
    }
    EXPECT_GT(checked,400);
}

TEST(CsgCrossingMembershipFilterContract, PovHeuristicKeepsUnionInternalSurfaces) {
    SimpleBody left, right;
    left.setGeometry(new Sphere(1)); right.setGeometry(new Sphere(1));
    right.setPosition(Vector3Dd(1.5,0,0));
    CsgBodyOperand a(left), b(right);
    CsgTree unionTree(CsgOperation::UNION), intersection(CsgOperation::INTERSECTION),
        difference(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(unionTree.addOperand(a)); ASSERT_TRUE(unionTree.addOperand(b));
    ASSERT_TRUE(intersection.addOperand(a)); ASSERT_TRUE(intersection.addOperand(b));
    ASSERT_TRUE(difference.addOperand(a)); ASSERT_TRUE(difference.addOperand(b));
    CsgRaySurfaceEvents events;

    // Union keeps every operand event; Roth reports only 2 and 5.5.
    ASSERT_EQ(RaySolidQueryStatus::Supported, CsgCrossingMembershipFilter::select(
        unionTree,alongX,RayCrossingQuery(0,20),1e-9,events));
    ASSERT_EQ(4,events.size());
    const double unionT[4]={2,3.5,4,5.5};
    for (int i=0;i<4;++i) EXPECT_DOUBLE_EQ(unionT[i],events.getT(i));

    ASSERT_EQ(RaySolidQueryStatus::Supported, CsgCrossingMembershipFilter::select(
        intersection,alongX,RayCrossingQuery(0,20),1e-9,events));
    ASSERT_EQ(2,events.size());
    EXPECT_DOUBLE_EQ(3.5,events.getT(0)); EXPECT_DOUBLE_EQ(4,events.getT(1));
    EXPECT_EQ(1,events.get(0)->leafIndex); EXPECT_EQ(0,events.get(1)->leafIndex);

    ASSERT_EQ(RaySolidQueryStatus::Supported, CsgCrossingMembershipFilter::select(
        difference,alongX,RayCrossingQuery(0,20),1e-9,events));
    ASSERT_EQ(2,events.size());
    EXPECT_DOUBLE_EQ(2,events.getT(0)); EXPECT_DOUBLE_EQ(3.5,events.getT(1));
    const CsgRayEventReference* hole=events.get(1);
    EXPECT_TRUE(events.getLeaves().get(hole->leafIndex)->getClassification().boundaries[hole->boundaryIndex].normalReversed);
}

TEST(CsgCrossingMembershipFilterContract, BoundaryMembershipKeepsCoincidentFacesAndUncertaintyFails) {
    SimpleBody first, same;
    first.setGeometry(new Box(2,2,2)); same.setGeometry(new Box(2,2,2));
    CsgBodyOperand a(first), c(same);
    CsgTree identical(CsgOperation::INTERSECTION);
    ASSERT_TRUE(identical.addOperand(a)); ASSERT_TRUE(identical.addOperand(c));
    CsgRaySurfaceEvents events;
    // Both coincident faces satisfy the boundary-inclusive membership; Roth
    // reports a single boundary for each pair.
    ASSERT_EQ(RaySolidQueryStatus::Supported, CsgCrossingMembershipFilter::select(
        identical,alongX,RayCrossingQuery(0,20),1e-9,events));
    ASSERT_EQ(4,events.size());
    EXPECT_DOUBLE_EQ(2,events.getT(0)); EXPECT_DOUBLE_EQ(2,events.getT(1));
    EXPECT_EQ(0,events.get(0)->leafIndex); EXPECT_EQ(1,events.get(1)->leafIndex);

    UncertifiedTestBody opaque; opaque.setGeometry(new Sphere(1));
    opaque.setPosition(Vector3Dd(0.5,0,0));
    CsgBodyOperand u(opaque);
    CsgTree uncertain(CsgOperation::INTERSECTION);
    ASSERT_TRUE(uncertain.addOperand(a)); ASSERT_TRUE(uncertain.addOperand(u));
    EXPECT_EQ(RaySolidQueryStatus::Unsupported, CsgCrossingMembershipFilter::select(
        uncertain,alongX,RayCrossingQuery(0,20),1e-9,events));
    EXPECT_EQ(0,events.size());

    // A proven exclusion by another sibling makes the uncertain one irrelevant.
    SimpleBody far; far.setGeometry(new Sphere(0.5)); far.setPosition(Vector3Dd(0,5,0));
    CsgBodyOperand f(far);
    CsgTree excluded(CsgOperation::INTERSECTION);
    ASSERT_TRUE(excluded.addOperand(a)); ASSERT_TRUE(excluded.addOperand(u)); ASSERT_TRUE(excluded.addOperand(f));
    CsgTree probe(CsgOperation::UNION);
    ASSERT_TRUE(probe.addOperand(a));
    EXPECT_EQ(RaySolidQueryStatus::Supported, CsgCrossingMembershipFilter::select(
        excluded,alongX,RayCrossingQuery(0,20),1e-9,events));
    EXPECT_EQ(0,events.size());
    EXPECT_EQ(RaySolidQueryStatus::InvalidInput, CsgCrossingMembershipFilter::select(
        probe,alongX,RayCrossingQuery(0,20),-1,events));
}

#include "vsdk/toolkit/environment/scene/csg/CsgPointProof.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointAttribution.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRayWorkspace.h"
#include "vsdk/toolkit/environment/scene/csg/CsgVersionStamp.h"
TEST(CsgVersionStampContract, ResultsDetectEditsOfBorrowedOperands) {
    SimpleBody left, right;
    left.setGeometry(new Sphere(1)); right.setGeometry(new Sphere(1));
    right.setPosition(Vector3Dd(1.5,0,0));
    CsgBodyOperand a(left), b(right);
    CsgTree inner(CsgOperation::UNION);
    ASSERT_TRUE(inner.addOperand(a)); ASSERT_TRUE(inner.addOperand(b));
    CsgPlacedExpression placed(inner);
    CsgTree root(CsgOperation::UNION);
    ASSERT_TRUE(root.addOperand(placed));
    CsgRayClassification result;
    ASSERT_EQ(RaySolidQueryStatus::Supported,
        CsgRothRayClassifier::classify(root,alongX,RayCrossingQuery(0,20),result));
    EXPECT_TRUE(result.getVersions().isComplete());
    EXPECT_TRUE(result.getVersions().isCurrent());
    right.setPosition(Vector3Dd(1.4,0,0));
    EXPECT_FALSE(result.getVersions().isCurrent());
    ASSERT_EQ(RaySolidQueryStatus::Supported,
        CsgRothRayClassifier::classify(root,alongX,RayCrossingQuery(0,20),result));
    EXPECT_TRUE(result.getVersions().isCurrent());
    ASSERT_EQ(AffinePlacementStatus::Valid,placed.setAffinePlacement(
        Matrix4x4d().translation(1,0,0),Matrix4x4d().translation(-1,0,0)));
    EXPECT_FALSE(result.getVersions().isCurrent());
    ASSERT_EQ(RaySolidQueryStatus::Supported,
        CsgRothRayClassifier::classify(root,alongX,RayCrossingQuery(0,20),result));
    SimpleBody extra; extra.setGeometry(new Sphere(0.5));
    CsgBodyOperand c(extra);
    ASSERT_TRUE(inner.addOperand(c));
    EXPECT_FALSE(result.getVersions().isCurrent());

    CsgPointProof proof;
    EXPECT_EQ(GeometryContainment::Inside,
        CsgPointClassification::classifyWithProof(root,Vector3Dd(1,0,0),0.01,proof));
    ASSERT_TRUE(proof.isAvailable());
    EXPECT_TRUE(proof.getVersions().isCurrent());
    left.setPosition(Vector3Dd(0,0.1,0));
    EXPECT_FALSE(proof.getVersions().isCurrent());

    CsgRaySurfaceEvents events;
    ASSERT_EQ(RaySolidQueryStatus::Supported, CsgCrossingMembershipFilter::select(
        root,alongX,RayCrossingQuery(0,20),1e-9,events));
    EXPECT_TRUE(events.getVersions().isCurrent());
    extra.setPosition(Vector3Dd(0,0.2,0));
    EXPECT_FALSE(events.getVersions().isCurrent());

    // A view that does not track its edits can never be proven current.
    CsgTestOperand untracked(GeometryContainment::Inside);
    CsgTree opaque(CsgOperation::UNION);
    ASSERT_TRUE(opaque.addOperand(a)); ASSERT_TRUE(opaque.addOperand(untracked));
    CsgVersionStamp stamp;
    ASSERT_TRUE(stamp.capture(opaque));
    EXPECT_TRUE(stamp.isCaptured());
    EXPECT_FALSE(stamp.isComplete());
    EXPECT_FALSE(stamp.isCurrent());
}

TEST(CsgRayWorkspaceContract, ReusedStorageGivesFreshResults) {
    SimpleBody outer, hole, side;
    outer.setGeometry(new Sphere(2)); hole.setGeometry(new Sphere(1)); side.setGeometry(new Box(1,1,1));
    side.setPosition(Vector3Dd(1.5,0,0));
    CsgBodyOperand a(outer), b(hole), c(side);
    CsgTree shell(CsgOperation::DIFFERENCE), small(CsgOperation::UNION);
    ASSERT_TRUE(shell.addOperand(a)); ASSERT_TRUE(shell.addOperand(b)); ASSERT_TRUE(shell.addOperand(c));
    ASSERT_TRUE(small.addOperand(b));
    CsgRayWorkspace workspace;
    CsgRayClassification reused;
    const CsgTree* order[5]={&shell,&small,&shell,&small,&shell};
    for (int i=0;i<5;++i) {
        CsgRayClassification fresh;
        const Ray ray=Ray::fromAffine(Vector3Dd(-5,0.1*i,0),Vector3Dd(1,0,0));
        ASSERT_EQ(RaySolidQueryStatus::Supported,
            CsgRothRayClassifier::classify(*order[i],ray,RayCrossingQuery(0,20),fresh));
        ASSERT_EQ(RaySolidQueryStatus::Supported,
            CsgRothRayClassifier::classify(*order[i],ray,RayCrossingQuery(0,20),workspace,reused));
        ASSERT_EQ(fresh.getSolid().boundaries.size(),reused.getSolid().boundaries.size());
        ASSERT_EQ(fresh.getSolid().insideSegments.size(),reused.getSolid().insideSegments.size());
        ASSERT_EQ(fresh.getLeaves().size(),reused.getLeaves().size());
        for (long int k=0;k<fresh.getSolid().boundaries.size();++k) {
            EXPECT_EQ(fresh.getSolid().boundaries[k].t,reused.getSolid().boundaries[k].t);
            EXPECT_EQ(fresh.getSolid().boundaries[k].normalReversed,reused.getSolid().boundaries[k].normalReversed);
            EXPECT_EQ(fresh.getSolid().boundaries[k].getInstanceCount(),reused.getSolid().boundaries[k].getInstanceCount());
            ASSERT_EQ(fresh.getOwnerCount(k),reused.getOwnerCount(k));
            EXPECT_EQ(fresh.getOwner(k,0)->leafIndex,reused.getOwner(k,0)->leafIndex);
        }
        for (long int k=0;k<fresh.getLeaves().size();++k)
            EXPECT_EQ(fresh.getLeaves().get(k)->getPath().getLength(),reused.getLeaves().get(k)->getPath().getLength());
    }
    CsgTestOperand uncertain(GeometryContainment::Indeterminate);
    CsgTree failing(CsgOperation::UNION);
    ASSERT_TRUE(failing.addOperand(a)); ASSERT_TRUE(failing.addOperand(uncertain));
    EXPECT_EQ(RaySolidQueryStatus::Unsupported,
        CsgRothRayClassifier::classify(failing,alongX,RayCrossingQuery(0,20),workspace,reused));
    EXPECT_EQ(0,reused.getLeaves().size());
    EXPECT_EQ(0,reused.getSolid().insideSegments.size());
    EXPECT_FALSE(reused.getVersions().isCaptured());
}
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/material/SimpleMaterial.h"
#include "vsdk/toolkit/environment/scene/csg/CsgBody.h"
TEST(CsgBodyContract, DifferenceHitsCarryLeafNormalsMaterialsAndPlacement) {
    CsgBody body(CsgOperation::DIFFERENCE,CsgBodyEvaluator::ROTH);
    SimpleBody* outer=new SimpleBody(); outer->setGeometry(new Sphere(2)); outer->setMaterial(new SimpleMaterial());
    SimpleBody* hole=new SimpleBody(); hole->setGeometry(new Sphere(1)); hole->setMaterial(new SimpleMaterial());
    hole->setPosition(Vector3Dd(1.5,0,0));
    ASSERT_TRUE(body.addLeaf(0,outer)); ASSERT_TRUE(body.addLeaf(0,hole));
    EXPECT_FALSE(body.addLeaf(3,new SimpleBody()));
    EXPECT_EQ(-1,body.addNode(5,CsgOperation::UNION));
    ASSERT_EQ(2,body.getLeafCount());
    RayHit hit(RayHit::DETAIL_POINT|RayHit::DETAIL_NORMAL,false);
    ASSERT_TRUE(body.doIntersectionFirstHit(Ray::fromAffine(Vector3Dd(-5,0,0),Vector3Dd(1,0,0)),&hit));
    EXPECT_NEAR(3,hit.getHitDistance(),1e-12);
    EXPECT_NEAR(-1,hit.normal.x(),1e-12); EXPECT_NEAR(-2,hit.point.x(),1e-12);
    EXPECT_EQ(outer->getMaterial(),hit.material);
    hit.reset(RayHit::DETAIL_POINT|RayHit::DETAIL_NORMAL);
    ASSERT_TRUE(body.doIntersectionFirstHit(Ray::fromAffine(Vector3Dd(5,0,0),Vector3Dd(-1,0,0)),&hit));
    EXPECT_NEAR(4.5,hit.getHitDistance(),1e-12);
    EXPECT_NEAR(1,hit.normal.x(),1e-12);
    EXPECT_EQ(hole->getMaterial(),hit.material);
    body.setPosition(Vector3Dd(0,0,10));
    EXPECT_FALSE(body.doIntersectionFirstHit(Ray::fromAffine(Vector3Dd(-5,0,0),Vector3Dd(1,0,0)),&hit));
    hit.reset(RayHit::DETAIL_POINT|RayHit::DETAIL_NORMAL);
    ASSERT_TRUE(body.doIntersectionFirstHit(Ray::fromAffine(Vector3Dd(-5,0,10),Vector3Dd(1,0,0)),&hit));
    EXPECT_NEAR(3,hit.getHitDistance(),1e-12); EXPECT_NEAR(10,hit.point.z(),1e-12);
    GeometryBounds bounds; body.getWorldBounds(bounds);
    ASSERT_EQ(GeometryBounds::Kind::Finite,bounds.getKind());
    EXPECT_LE(bounds.getMinimum().z(),8); EXPECT_GE(bounds.getMaximum().z(),12);
    EXPECT_EQ(GeometryContainment::Inside,body.classifyPointCertified(Vector3Dd(-1,0,10),1e-9));
    EXPECT_EQ(GeometryContainment::Outside,body.classifyPointCertified(Vector3Dd(1.5,0,10),1e-9));
    RaySolidClassification solid;
    ASSERT_EQ(RaySolidQueryStatus::Supported,
        body.classifyRay(Ray::fromAffine(Vector3Dd(-5,0,10),Vector3Dd(1,0,0)),RayCrossingQuery(0,20),solid));
    ASSERT_EQ(2,solid.boundaries.size());
    EXPECT_NEAR(3,solid.boundaries[0].t,1e-12); EXPECT_NEAR(5.5,solid.boundaries[1].t,1e-12);
}

TEST(CsgBodyContract, EvaluatorsDifferOnNestedUnionInternalSurfaces) {
    CsgBody* bodies[2]={new CsgBody(CsgOperation::DIFFERENCE,CsgBodyEvaluator::ROTH),
        new CsgBody(CsgOperation::DIFFERENCE,CsgBodyEvaluator::CROSSING_MEMBERSHIP)};
    for (int i=0;i<2;++i) {
        SimpleBody* outer=new SimpleBody(); outer->setGeometry(new Sphere(5));
        SimpleBody* left=new SimpleBody(); left->setGeometry(new Sphere(2)); left->setPosition(Vector3Dd(-1,0,0));
        SimpleBody* right=new SimpleBody(); right->setGeometry(new Sphere(2)); right->setPosition(Vector3Dd(1,0,0));
        ASSERT_TRUE(bodies[i]->addLeaf(0,outer));
        const long int holes=bodies[i]->addNode(0,CsgOperation::UNION);
        ASSERT_EQ(1,holes);
        ASSERT_TRUE(bodies[i]->addLeaf(holes,left)); ASSERT_TRUE(bodies[i]->addLeaf(holes,right));
        RayHit outside(RayHit::DETAIL_NONE,false);
        ASSERT_TRUE(bodies[i]->doIntersectionFirstHit(Ray::fromAffine(Vector3Dd(-10,0,0),Vector3Dd(1,0,0)),&outside));
        EXPECT_NEAR(5,outside.getHitDistance(),1e-12);
    }
    RayHit roth(RayHit::DETAIL_POINT|RayHit::DETAIL_NORMAL,false), pov(RayHit::DETAIL_POINT|RayHit::DETAIL_NORMAL,false);
    const Ray fromHole=Ray::fromAffine(Vector3Dd(0,0,0),Vector3Dd(1,0,0));
    ASSERT_TRUE(bodies[0]->doIntersectionFirstHit(fromHole,&roth));
    ASSERT_TRUE(bodies[1]->doIntersectionFirstHit(fromHole,&pov));
    EXPECT_NEAR(3,roth.getHitDistance(),1e-12);
    EXPECT_NEAR(1,pov.getHitDistance(),1e-12);
    EXPECT_NEAR(-1,roth.normal.x(),1e-12);
    EXPECT_NEAR(-1,pov.normal.x(),1e-12);
    delete bodies[0]; delete bodies[1];
}

TEST(CsgMembershipRuleContract, FoldStopsOnlyWhenLaterOperandsCannotChangeIt) {
    const CsgOperation operations[] = { CsgOperation::UNION, CsgOperation::INTERSECTION,
        CsgOperation::DIFFERENCE };
    for ( int k = 0; k < 3; k++ ) {
        for ( int accumulated = 0; accumulated < 2; accumulated++ ) {
            const bool value = accumulated == 1;
            const bool unchanged = CsgMembershipRule::combine(operations[k], value, false) == value &&
                CsgMembershipRule::combine(operations[k], value, true) == value;
            EXPECT_EQ(CsgMembershipRule::isDecided(operations[k], value), unchanged);
        }
    }
}

TEST(CsgSegmentClassificationContract, MergesLeafSegmentsByTableThree) {
    // Left solid [1,3], right solid [2,4]; payloads name the events.
    CsgSegmentClassification<int> left(false, 2), right(false, 2);
    left.append(1, 10); left.append(3, 11);
    right.append(2, 20); right.append(4, 21);
    ASSERT_TRUE(left.getEvents()[0].isEntering());
    ASSERT_FALSE(left.getEvents()[1].isEntering());
    const CsgSegmentClassification<int> u =
        CsgSegmentClassification<int>::merge(left, right, CsgOperation::UNION);
    ASSERT_EQ(2, u.getEvents().size());
    EXPECT_EQ(10, u.getEvents()[0].getPayload()); EXPECT_EQ(21, u.getEvents()[1].getPayload());
    const CsgSegmentClassification<int> i =
        CsgSegmentClassification<int>::merge(left, right, CsgOperation::INTERSECTION);
    ASSERT_EQ(2, i.getEvents().size());
    EXPECT_EQ(20, i.getEvents()[0].getPayload()); EXPECT_EQ(11, i.getEvents()[1].getPayload());
    const CsgSegmentClassification<int> d =
        CsgSegmentClassification<int>::merge(left, right, CsgOperation::DIFFERENCE);
    ASSERT_EQ(2, d.getEvents().size());
    EXPECT_EQ(10, d.getEvents()[0].getPayload()); EXPECT_TRUE(d.getEvents()[0].isEntering());
    EXPECT_EQ(20, d.getEvents()[1].getPayload()); EXPECT_FALSE(d.getEvents()[1].isEntering());
    EXPECT_FALSE(d.isInitialInside());
}

TEST(CsgSegmentClassificationContract, InitialStateAndTiesTakeTheLeftEvent) {
    // A ray starting inside the left solid, leaving it where the right one ends.
    CsgSegmentClassification<int> left(true, 1), right(true, 1);
    left.append(2, 1); right.append(2, 2);
    const CsgSegmentClassification<int> u =
        CsgSegmentClassification<int>::merge(left, right, CsgOperation::UNION);
    EXPECT_TRUE(u.isInitialInside());
    ASSERT_EQ(1, u.getEvents().size());
    EXPECT_EQ(2, u.getEvents()[0].getPayload());
    const CsgSegmentClassification<int> i =
        CsgSegmentClassification<int>::merge(left, right, CsgOperation::INTERSECTION);
    ASSERT_EQ(1, i.getEvents().size());
    EXPECT_EQ(1, i.getEvents()[0].getPayload());
}

TEST(CsgTreeContract, OperationEditsAreStructureEdits) {
    CsgTree tree(CsgOperation::UNION);
    long long before = 0, after = 0;
    ASSERT_TRUE(tree.queryStructureVersion(before));
    tree.setOperation(CsgOperation::INTERSECTION);
    ASSERT_TRUE(tree.queryStructureVersion(after));
    EXPECT_EQ(CsgOperation::INTERSECTION, tree.getOperation());
    EXPECT_NE(before, after);
}

TEST(CsgBodyOperandContract, StructuralTraversalFollowsThePlacedGeometryOptIn) {
    SimpleBody plain; plain.setGeometry(new Sphere(1));
    CsgBody composed(CsgOperation::UNION, CsgBodyEvaluator::ROTH);
    CsgBodyOperand plainView(plain), composedView(composed);
    EXPECT_TRUE(plain.isStructuralTraversalSafe());
    EXPECT_TRUE(plainView.isStructuralTraversalSafe());
    EXPECT_FALSE(composedView.isStructuralTraversalSafe());
}

#include <gtest/gtest.h>
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingQuery.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/scene/csg/CsgBodyOperand.h"
#include "vsdk/toolkit/environment/scene/csg/CsgCrossingMembershipFilter.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPlacedExpression.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointClassification.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointWorkspace.h"
#include "vsdk/toolkit/environment/scene/csg/CsgRaySurfaceEvents.h"
#include "vsdk/toolkit/environment/scene/csg/CsgTree.h"
#include "UncertifiedTestBody.h"

// Random trees with repeated operands, a sheared placed subtree, an uncertain
// operand and points on and near boundaries: the containment-only query must
// give the result of the full classification for every point and tolerance.
TEST(CsgPointWorkspaceContract, ContainmentMatchesFullClassificationOnRandomTrees) {
    unsigned int seed=97531;
    const int bodyCount=5;
    SimpleBody bodies[bodyCount];
    for (int i=0;i<bodyCount;++i) {
        if (i%2==0) bodies[i].setGeometry(new Sphere(0.6+0.1*i));
        else bodies[i].setGeometry(new Box(1.0+0.2*i,1.4,0.9+0.1*i));
    }
    UncertifiedTestBody opaque;
    opaque.setGeometry(new Sphere(0.7));
    const double tolerances[3]={0,1e-6,0.05};
    const double shear[4][4]={{1.3,0.2,0,0.1},{0,0.8,0.3,-0.2},{0.1,0,1.1,0.05},{0,0,0,1}};
    const Matrix4x4d forward(shear);
    CsgPointWorkspace workspace;
    long int compared=0, definite=0;
    for (int trial=0;trial<120;++trial) {
        for (int i=0;i<bodyCount;++i) {
            seed=seed*1103515245u+12345u; const double x=((seed>>8)%2000)/1000.0-1.0;
            seed=seed*1103515245u+12345u; const double y=((seed>>8)%2000)/2000.0-0.5;
            bodies[i].setPosition(Vector3Dd(x,y,0.1*i-0.2));
        }
        opaque.setPosition(Vector3Dd(0.3*(trial%5)-0.6,0,0));
        CsgBodyOperand o0(bodies[0]), o1(bodies[1]), o2(bodies[2]), o3(bodies[3]), o4(bodies[4]);
        CsgBodyOperand again(bodies[1]), u(opaque);
        const CsgOperation ops[3]={CsgOperation::UNION,CsgOperation::INTERSECTION,CsgOperation::DIFFERENCE};
        CsgTree inner(ops[trial%3]), middle(ops[(trial/3)%3]), root(ops[(trial/9)%3]);
        ASSERT_TRUE(inner.addOperand(o0)); ASSERT_TRUE(inner.addOperand(o1)); ASSERT_TRUE(inner.addOperand(again));
        if (trial%4==0) ASSERT_TRUE(inner.addOperand(u));
        CsgPlacedExpression placed(inner);
        ASSERT_EQ(AffinePlacementStatus::Valid,
            placed.setAffinePlacement(trial%2 ? forward : Matrix4x4d::identityMatrix(),
                trial%2 ? forward.inverse() : Matrix4x4d::identityMatrix()));
        ASSERT_TRUE(middle.addOperand(o3)); ASSERT_TRUE(middle.addOperand(placed));
        ASSERT_TRUE(root.addOperand(middle)); ASSERT_TRUE(root.addOperand(o4)); ASSERT_TRUE(root.addOperand(o2));
        if (trial%6==0) ASSERT_TRUE(root.addOperand(o2));
        for (int k=0;k<40;++k) {
            Vector3Dd p;
            if (k<20) {
                seed=seed*1103515245u+12345u; const double x=((seed>>8)%3000)/1000.0-1.5;
                seed=seed*1103515245u+12345u; const double y=((seed>>8)%2000)/1000.0-1.0;
                p=Vector3Dd(x,y,0.05);
            }
            else {
                // Points on the spheres and box faces.
                const SimpleBody& b=bodies[k%bodyCount];
                const double r=k%2==0 ? 0.6+0.1*(k%bodyCount) : 0.5*(1.0+0.2*(k%bodyCount));
                p=b.getPosition().add(Vector3Dd(r,0,0));
            }
            for (int t=0;t<3;++t) {
                const GeometryContainment full=CsgPointClassification::classify(root,p,tolerances[t]);
                const GeometryContainment lean=
                    CsgPointClassification::classifyContainment(root,p,tolerances[t],workspace);
                EXPECT_EQ(full,lean) << "trial " << trial << " point " << k << " tolerance " << t;
                ++compared;
                if (full==GeometryContainment::Inside || full==GeometryContainment::Outside ||
                    full==GeometryContainment::Boundary) ++definite;
                EXPECT_EQ(0,workspace.results.size());
                EXPECT_EQ(0,workspace.path.size());
            }
        }
    }
    EXPECT_GT(definite,compared/2);
    EXPECT_LT(definite,compared);
}

TEST(CsgPointWorkspaceContract, OperandContainmentAndInvalidInput) {
    SimpleBody ball;
    ball.setGeometry(new Sphere(1));
    CsgBodyOperand a(ball);
    CsgPointWorkspace workspace;
    EXPECT_EQ(GeometryContainment::Inside,
        CsgPointClassification::classifyOperandContainment(a,Vector3Dd(0,0,0),0,workspace));
    EXPECT_EQ(GeometryContainment::Boundary,
        CsgPointClassification::classifyOperandContainment(a,Vector3Dd(1,0,0),1e-9,workspace));
    EXPECT_EQ(GeometryContainment::Outside,
        CsgPointClassification::classifyOperandContainment(a,Vector3Dd(3,0,0),0,workspace));
    EXPECT_EQ(GeometryContainment::InvalidInput,
        CsgPointClassification::classifyOperandContainment(a,Vector3Dd(0,0,0),-1,workspace));
    CsgTree tree(CsgOperation::UNION);
    ASSERT_TRUE(tree.addOperand(a));
    EXPECT_EQ(GeometryContainment::InvalidInput,
        CsgPointClassification::classifyContainment(tree,Vector3Dd(0,java::Double::NaN,0),0,workspace));
}

// The heuristic gives the same events with a reused workspace as with a fresh
// one, query after query.
TEST(CsgPointWorkspaceContract, HeuristicSelectionReusesTheWorkspace) {
    SimpleBody a, b, c;
    a.setGeometry(new Sphere(2)); b.setGeometry(new Sphere(1)); c.setGeometry(new Sphere(1));
    c.setPosition(Vector3Dd(1.5,0,0));
    CsgBodyOperand oa(a), ob(b), oc(c);
    CsgTree inner(CsgOperation::UNION), root(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(inner.addOperand(ob)); ASSERT_TRUE(inner.addOperand(oc));
    ASSERT_TRUE(root.addOperand(oa)); ASSERT_TRUE(root.addOperand(inner));
    CsgPointWorkspace workspace;
    CsgRaySurfaceEvents fresh, reused;
    for (int i=0;i<20;++i) {
        const Ray ray=Ray::fromAffine(Vector3Dd(-5,0.1*(i%7)-0.3,0.05*(i%3)),Vector3Dd(1,0,0));
        ASSERT_EQ(RaySolidQueryStatus::Supported,
            CsgCrossingMembershipFilter::select(root,ray,RayCrossingQuery(0,20),1e-9,fresh));
        ASSERT_EQ(RaySolidQueryStatus::Supported,
            CsgCrossingMembershipFilter::select(root,ray,RayCrossingQuery(0,20),1e-9,workspace,reused));
        ASSERT_EQ(fresh.size(),reused.size());
        for (long int k=0;k<fresh.size();++k) {
            EXPECT_EQ(fresh.get(k)->leafIndex,reused.get(k)->leafIndex);
            EXPECT_EQ(fresh.get(k)->boundaryIndex,reused.get(k)->boundaryIndex);
            EXPECT_DOUBLE_EQ(fresh.getT(k),reused.getT(k));
        }
        // Difference: both outer crossings plus the hole crossings inside the ball.
        EXPECT_EQ(4,reused.size());
    }
}

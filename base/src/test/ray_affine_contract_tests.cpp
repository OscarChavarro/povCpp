#include <gtest/gtest.h>
#include <fenv.h>
#include "java/lang/Double.h"
#include "java/lang/IllegalStateException.h"
#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "RayCrossingTestCollector.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/Triangle.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/Torus.h"
#include "vsdk/toolkit/environment/geometry/volume/PlanarHalfSpace.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/geometry/volume/OrthogonalEllipsoidDistance.h"
#include "vsdk/toolkit/environment/geometry/volume/AffineEllipsoidDistance.h"
#include "vsdk/toolkit/environment/geometry/volume/AffineEllipsoidDistanceBounds.h"
#include "vsdk/toolkit/environment/geometry/volume/AffineEllipsoidMinimumDistance.h"
#include "vsdk/toolkit/common/numerics/FloatingInterval.h"

TEST(FloatingIntervalContract, OutwardRoundingAndFailureCases) {
    ASSERT_TRUE(FloatingInterval::isSupportedEnvironment());
    const FloatingInterval sum=FloatingInterval::exact(0.1).add(FloatingInterval::exact(0.2));
    ASSERT_TRUE(sum.isValid());
    EXPECT_LE(sum.getLower(),0.3);
    EXPECT_GE(sum.getUpper(),0.3);
    const FloatingInterval quotient=FloatingInterval::exact(1).divide(FloatingInterval::exact(3));
    ASSERT_TRUE(quotient.isValid());
    EXPECT_LT(quotient.getLower(),1.0/3.0);
    EXPECT_GT(quotient.getUpper(),1.0/3.0);
    EXPECT_FALSE(FloatingInterval::exact(1).divide(FloatingInterval::exact(0)).isValid());
    EXPECT_FALSE(FloatingInterval::exact(java::Double::MAX_VALUE).multiply(FloatingInterval::exact(2)).isValid());
    const FloatingInterval tiny=FloatingInterval::exact(java::Double::MIN_VALUE).multiply(FloatingInterval::exact(0.5));
    ASSERT_TRUE(tiny.isValid());
    EXPECT_LE(tiny.getLower(),0);
    const double minValue=java::Double::MIN_VALUE;
    EXPECT_GE(tiny.getUpper(),minValue);
    const int oldMode=fegetround();
    const int changed=fesetround(FE_UPWARD);
    const bool supported=FloatingInterval::isSupportedEnvironment();
    const int restored=fesetround(oldMode);
    EXPECT_EQ(0,changed); EXPECT_FALSE(supported); EXPECT_EQ(0,restored);
}

TEST(FloatingIntervalContract, BoundsContainIndependentExactRationalResults) {
    ASSERT_TRUE(FloatingInterval::isSupportedEnvironment());
    // Selected floor and ceiling endpoints from
    // generate_interval_reference.py, using exact rational arithmetic.
    const double cases[][5]={
        {0.1,0.2,0,0.3,0.30000000000000004},
        {0.1,0.2,2,0.02,0.020000000000000004},
        {-0.1,0.2,1,-0.30000000000000004,-0.3},
        {1e-300,1e-20,0,1e-20,1.0000000000000001e-20},
        {1e-300,1e-20,2,1e-320,1.0005e-320},
        {1e-300,1e-20,3,1.0000000000000001e-280,1.0000000000000002e-280},
        {1e150,1e140,1,9.999999998999999e149,9.999999999e149},
        {1e150,1e140,2,9.999999999999999e289,1e290},
        {1.0000000000000002,0.9999999999999999,1,3.3306690738754696e-16,3.3306690738754696e-16},
        {1.0000000000000002,0.9999999999999999,3,1.0000000000000002,1.0000000000000004},
        {-1e-200,-1e-110,2,1e-310,1.00000000000005e-310},
        {1,3,3,0.3333333333333333,0.33333333333333337},
        {1e-200,1e100,3,9.999999999999999e-301,1e-300}
    };
    for (int i=0;i<13;++i) {
        const FloatingInterval a=FloatingInterval::exact(cases[i][0]);
        const FloatingInterval b=FloatingInterval::exact(cases[i][1]);
        FloatingInterval result;
        switch (static_cast<int>(cases[i][2])) {
        case 0: result=a.add(b); break;
        case 1: result=a.subtract(b); break;
        case 2: result=a.multiply(b); break;
        default: result=a.divide(b); break;
        }
        ASSERT_TRUE(result.isValid()) << i;
        EXPECT_LE(result.getLower(),cases[i][3]) << i;
        EXPECT_GE(result.getUpper(),cases[i][4]) << i;
    }
}

TEST(EllipsoidDistanceBoundsContract, AxisAndShearEncloseKnownDistances) {
    double lower=-1,upper=-1;
    GeometryContainment side=GeometryContainment::Indeterminate;
    const Matrix4x4d identity=Matrix4x4d::identityMatrix();
    ASSERT_EQ(GeometryDistanceStatus::Valid,AffineEllipsoidDistanceBounds::query(
        Vector3Dd(3,0,0),1,identity,Vector3Dd(1,0,0),side,lower,upper));
    EXPECT_EQ(GeometryContainment::Outside,side);
    EXPECT_LE(lower,2); EXPECT_GE(upper,2);
    EXPECT_NEAR(2,lower,1e-12); EXPECT_NEAR(2,upper,1e-12);
    ASSERT_EQ(GeometryDistanceStatus::Valid,AffineEllipsoidDistanceBounds::query(
        Vector3Dd(),1,identity,Vector3Dd(0,1,0),side,lower,upper));
    EXPECT_EQ(GeometryContainment::Inside,side);
    EXPECT_GT(lower,0); EXPECT_LE(lower,1); EXPECT_GE(upper,1); EXPECT_NEAR(1,upper,1e-12);
    const Matrix4x4d shear=Matrix4x4d().translation(3,-2,1).multiply(
        Matrix4x4d::identityMatrix().withVal(0,0,2).withVal(0,1,0.5));
    const Vector3Dd seed(0.6,0.8,0),normal(0.3,0.65,0);
    const Vector3Dd point=shear.multiply(seed).add(normal.multiply(0.2/normal.length()));
    ASSERT_EQ(GeometryDistanceStatus::Valid,
        AffineEllipsoidDistanceBounds::query(point,1,shear,seed,side,lower,upper));
    EXPECT_EQ(GeometryContainment::Outside,side);
    EXPECT_LE(lower,0.2+1e-12); EXPECT_GE(upper,0.2-1e-12);
    EXPECT_NEAR(0.2,upper,1e-10);
}

TEST(EllipsoidDistanceBoundsContract, InvalidUncertainAndRoundingModeKeepOutputs) {
    double lower=17,upper=23;
    GeometryContainment side=GeometryContainment::Boundary;
    EXPECT_EQ(GeometryDistanceStatus::InvalidInput,AffineEllipsoidDistanceBounds::query(
        Vector3Dd(),1,Matrix4x4d().scale(0,1,1),Vector3Dd(1,0,0),side,lower,upper));
    EXPECT_EQ(GeometryDistanceStatus::InvalidInput,AffineEllipsoidDistanceBounds::query(
        Vector3Dd(),1,Matrix4x4d(),Vector3Dd(),side,lower,upper));
    EXPECT_DOUBLE_EQ(17,lower); EXPECT_DOUBLE_EQ(23,upper);
    EXPECT_EQ(GeometryContainment::Boundary,side);
    const Matrix4x4d ill=Matrix4x4d::identityMatrix().withVal(0,1,1).withVal(1,1,1e-18);
    const GeometryDistanceStatus status=AffineEllipsoidDistanceBounds::query(
        Vector3Dd(2,0,0),1,ill,Vector3Dd(1,0,0),side,lower,upper);
    EXPECT_TRUE(status==GeometryDistanceStatus::Indeterminate || status==GeometryDistanceStatus::Valid);
    if (status==GeometryDistanceStatus::Indeterminate) { EXPECT_DOUBLE_EQ(17,lower); EXPECT_DOUBLE_EQ(23,upper); }
    if (status==GeometryDistanceStatus::Valid) {
        const double distance=2-java::Math::sqrt(2.0);
        EXPECT_LE(lower,distance+1e-12); EXPECT_GE(upper,distance-1e-12);
    }
    lower=17; upper=23;
    side=GeometryContainment::Boundary;
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    const int oldMode=fegetround();
    const int changed=fesetround(FE_DOWNWARD);
    const GeometryDistanceStatus unsupported=AffineEllipsoidDistanceBounds::query(
        Vector3Dd(3,0,0),1,Matrix4x4d(),Vector3Dd(1,0,0),side,lower,upper);
    const GeometryContainment unsupportedContainment=body.classifyPoint(Vector3Dd(3,0,0),0.1);
    const GeometryContainment unsupportedIntrinsic=sphere.classifyPoint(Vector3Dd(3,0,0),0.1);
    const int restored=fesetround(oldMode);
    EXPECT_EQ(0,changed); EXPECT_EQ(0,restored);
    EXPECT_EQ(GeometryDistanceStatus::Unsupported,unsupported);
    EXPECT_EQ(GeometryContainment::Unsupported,unsupportedContainment);
    EXPECT_EQ(GeometryContainment::Unsupported,unsupportedIntrinsic);
    EXPECT_DOUBLE_EQ(17,lower); EXPECT_DOUBLE_EQ(23,upper);
    EXPECT_EQ(GeometryContainment::Boundary,side);
}

TEST(EllipsoidDistanceBoundsContract, SceneExposesCertifiedEnclosuresAndContainment) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    body.setScale(Vector3Dd(3,2,1));
    GeometryContainment side=GeometryContainment::Indeterminate;
    double lower=0,upper=0;
    ASSERT_EQ(GeometryDistanceStatus::Valid,body.encloseBoundaryDistance(
        Vector3Dd(3.2,0,0),Vector3Dd(1,0,0),side,lower,upper));
    EXPECT_EQ(GeometryContainment::Outside,side);
    EXPECT_LE(lower,0.2+1e-14); EXPECT_GE(upper,0.2-1e-14);
    EXPECT_EQ(GeometryContainment::Inside,body.classifyPointCertified(Vector3Dd(),0.1));
    // World distance is 0.2; the plain inverse-norm bound is below 0.14.
    EXPECT_EQ(GeometryContainment::Inside,body.classifyPointCertified(Vector3Dd(2.8,0,0),0.14));
    EXPECT_EQ(GeometryContainment::Outside,body.classifyPointCertified(Vector3Dd(3.2,0,0),0.1));
    EXPECT_EQ(GeometryContainment::Boundary,body.classifyPointCertified(Vector3Dd(3.05,0,0),0.1));
    EXPECT_EQ(GeometryContainment::Indeterminate,body.classifyPointCertified(Vector3Dd(3.1,0,0),0.1));
    EXPECT_EQ(GeometryContainment::InvalidInput,body.classifyPointCertified(Vector3Dd(),-1));
    Box box(1,1,1);
    body.setGeometryReference(&box);
    EXPECT_EQ(GeometryContainment::Unsupported,body.classifyPointCertified(Vector3Dd(),0.1));
}

TEST(EllipsoidDistanceBoundsContract, IndependentReferencesStayInsideCertifiedBounds) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    const Matrix4x4d rotation=Matrix4x4d::identityMatrix().withVal(0,0,0).withVal(0,1,-1)
        .withVal(1,0,1).withVal(1,1,0);
    const Matrix4x4d inverseRotation=Matrix4x4d::identityMatrix().withVal(0,0,0).withVal(0,1,1)
        .withVal(1,0,-1).withVal(1,1,0);
    const Matrix4x4d shape=Matrix4x4d::identityMatrix().withVal(0,0,-2).withVal(0,1,0.5).withVal(1,2,-0.3);
    const Matrix4x4d inverseShape=Matrix4x4d::identityMatrix().withVal(0,0,-0.5)
        .withVal(0,1,0.25).withVal(0,2,0.075).withVal(1,2,0.3);
    const double samples[][4]={{0.12,0.27,0.31,0.484219147169112},
        {0.65,0.1,-0.2,0.437976391530554},{1.1,0.2,0.3,0.228896833955284},
        {0.1,-1.2,0.5,0.341598824284725},{-0.9,-0.4,-0.3,0.0336629761551959},
        {0,0,0,0.84234037196832}};
    const double factors[]={0.01,1,100};
    int definite=0;
    for (int k=0;k<3;++k) {
        const double factor=factors[k];
        const Matrix4x4d forward=Matrix4x4d().translation(4,-3,2).multiply(rotation)
            .multiply(Matrix4x4d().scale(factor,factor,factor)).multiply(shape);
        const Matrix4x4d inverse=inverseShape.multiply(Matrix4x4d().scale(1/factor,1/factor,1/factor))
            .multiply(inverseRotation).multiply(Matrix4x4d().translation(-4,3,-2));
        ASSERT_EQ(AffinePlacementStatus::Valid,body.setAffinePlacement(forward,inverse));
        for (int index=0;index<6;++index) {
            const Vector3Dd local(samples[index][0],samples[index][1],samples[index][2]);
            const Vector3Dd point=forward.multiply(local);
            const Vector3Dd seed=local.dotProduct(local)>0 ? local : Vector3Dd(1,0,0);
            GeometryContainment side=GeometryContainment::Indeterminate;
            double lower=0,upper=0;
            ASSERT_EQ(GeometryDistanceStatus::Valid,
                body.encloseBoundaryDistance(point,seed,side,lower,upper)) << k << ", " << index;
            const double reference=samples[index][3]*factor;
            EXPECT_LE(lower,reference+1e-9*factor) << k << ", " << index;
            EXPECT_GE(upper,reference-1e-9*factor) << k << ", " << index;
            const GeometryContainment expected=local.dotProduct(local)<1
                ? GeometryContainment::Inside : GeometryContainment::Outside;
            EXPECT_TRUE(side==expected || side==GeometryContainment::Indeterminate);
            const double nearTolerance=reference+1e-4*factor;
            const GeometryContainment near=body.classifyPointCertified(point,nearTolerance);
            EXPECT_TRUE(near==GeometryContainment::Boundary || near==GeometryContainment::Indeterminate);
            EXPECT_EQ(near,body.classifyPoint(point,nearTolerance));
            const double farTolerance=reference*0.25;
            const GeometryContainment far=body.classifyPointCertified(point,farTolerance);
            EXPECT_TRUE(far==expected || far==GeometryContainment::Indeterminate);
            EXPECT_EQ(far,body.classifyPoint(point,farTolerance));
            if (near==GeometryContainment::Boundary) ++definite;
            if (far==expected) ++definite;
        }
    }
    EXPECT_EQ(36,definite);
}

TEST(EllipsoidDistanceBoundsContract, DiverseAffineFixturesEncloseIndependentDistances) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    const Matrix4x4d matrices[]={
        Matrix4x4d::identityMatrix().withVal(0,1,0.9).withVal(0,2,0.2).withVal(1,2,0.6),
        Matrix4x4d::identityMatrix().withVal(0,0,0.2).withVal(1,1,3).withVal(1,2,0.4),
        Matrix4x4d::identityMatrix().withVal(0,1,0.8).withVal(0,2,0.4).withVal(1,0,0.3)
            .withVal(1,2,-0.2).withVal(2,0,0.1).withVal(2,1,0.5).withVal(2,2,1.2)
    };
    const Vector3Dd points[]={Vector3Dd(),Vector3Dd(0.1,0.3,-0.2),Vector3Dd(0.6,-0.2,0.1),
        Vector3Dd(0.3,0.6,0.2),Vector3Dd(1.2,0.3,-0.1),Vector3Dd(-0.4,-1.1,0.6),
        Vector3Dd(0.2,0.1,1.4),Vector3Dd(-0.9,0.3,-0.4)};
    // Generated independently by generate_certified_bounds_reference.py.
    const double expected[][8]={
        {0.578406438716408,0.436864249515077,0.21684579558424,0.342292911715945,
         0.222512940034161,0.318320672587438,0.453659506464624,0.0181298663812327},
        {0.2,0.166279867729029,0.0749031414139726,0.0947459615938375,
         0.0502305824969421,0.439324293232699,0.413291190037398,0.00675839616866524},
        {0.526534747238471,0.427659995509968,0.195778365340438,0.309070492537248,
         0.199911925624104,0.356476528572591,0.522022480951273,0.0172612127525319}
    };
    int verifiedSides=0,definite=0;
    for (int matrix=0;matrix<3;++matrix) {
      ASSERT_EQ(AffinePlacementStatus::Valid,body.setAffinePlacement(matrices[matrix],matrices[matrix].inverse()));
      for (int index=0;index<8;++index) {
        const Vector3Dd point=matrices[matrix].multiply(points[index]);
        const Vector3Dd seed=points[index].dotProduct(points[index])>0 ? points[index] : Vector3Dd(1,0,0);
        const GeometryContainment expectedSide=points[index].dotProduct(points[index])<1
            ? GeometryContainment::Inside : GeometryContainment::Outside;
        GeometryContainment side=GeometryContainment::Indeterminate;
        double lower=0,upper=0;
        ASSERT_EQ(GeometryDistanceStatus::Valid,AffineEllipsoidDistanceBounds::query(
            point,1,matrices[matrix],seed,side,lower,upper)) << matrix << ", " << index;
        EXPECT_LE(lower,expected[matrix][index]+1e-8) << matrix << ", " << index;
        EXPECT_GE(upper,expected[matrix][index]-1e-8) << matrix << ", " << index;
        EXPECT_TRUE(side==expectedSide || side==GeometryContainment::Indeterminate);
        if (side==expectedSide) ++verifiedSides;
        const GeometryContainment near=body.classifyPointCertified(point,expected[matrix][index]+1e-4);
        EXPECT_TRUE(near==GeometryContainment::Boundary || near==GeometryContainment::Indeterminate);
        if (near==GeometryContainment::Boundary) ++definite;
        const GeometryContainment far=body.classifyPointCertified(point,expected[matrix][index]*0.25);
        EXPECT_TRUE(far==expectedSide || far==GeometryContainment::Indeterminate);
        if (far==expectedSide) ++definite;
        GeometryContainment minimumSide=GeometryContainment::Indeterminate;
        double minimumLower=0,minimumUpper=0;
        ASSERT_EQ(GeometryDistanceStatus::Valid,body.encloseMinimumBoundaryDistance(
            point,1e-5,minimumSide,minimumLower,minimumUpper)) << matrix << ", " << index;
        EXPECT_LE(minimumLower,expected[matrix][index]+1e-8) << matrix << ", " << index;
        EXPECT_GE(minimumUpper,expected[matrix][index]-1e-8) << matrix << ", " << index;
        EXPECT_LE(minimumUpper-minimumLower,1e-5);
      }
    }
    EXPECT_GE(verifiedSides,20);
    EXPECT_EQ(48,definite);
}

TEST(EllipsoidDistanceBoundsContract, NonUnitRadiusAndShearUseWorldTolerance) {
    Sphere sphere(2);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    const Matrix4x4d forward=Matrix4x4d().translation(3,-2,1).multiply(
        Matrix4x4d::identityMatrix().withVal(0,0,2).withVal(0,1,0.5));
    const Matrix4x4d inverse=Matrix4x4d::identityMatrix().withVal(0,0,0.5).withVal(0,1,-0.25)
        .multiply(Matrix4x4d().translation(-3,2,-1));
    ASSERT_EQ(AffinePlacementStatus::Valid,body.setAffinePlacement(forward,inverse));
    const Vector3Dd source(0.6,0.8,0),normal(0.3,0.65,0);
    const Vector3Dd point=forward.multiply(source.multiply(2)).add(normal.multiply(0.2/normal.length()));
    GeometryContainment side=GeometryContainment::Indeterminate;
    double lower=0,upper=0;
    ASSERT_EQ(GeometryDistanceStatus::Valid,body.encloseBoundaryDistance(point,source,side,lower,upper));
    EXPECT_EQ(GeometryContainment::Outside,side);
    EXPECT_LE(lower,0.2+1e-12); EXPECT_GE(upper,0.2-1e-12);
    EXPECT_EQ(GeometryContainment::Outside,body.classifyPointCertified(point,0.1));
    EXPECT_EQ(GeometryContainment::Boundary,body.classifyPointCertified(point,0.3));
    EXPECT_EQ(GeometryContainment::Inside,body.classifyPointCertified(Vector3Dd(3,-2,1),0.1));
}

TEST(EllipsoidMinimumDistanceContract, SceneProducesCertifiedTightDistances) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    body.setScale(Vector3Dd(3,2,1));
    const Vector3Dd points[]={Vector3Dd(3.2,0,0),Vector3Dd(),Vector3Dd(1,0,0),Vector3Dd(2.8,0,0)};
    const double expected[]={0.2,1,java::Math::sqrt(0.875),0.2};
    const GeometryContainment sides[]={GeometryContainment::Outside,GeometryContainment::Inside,
        GeometryContainment::Inside,GeometryContainment::Inside};
    for (int i=0;i<4;++i) {
        GeometryContainment side=GeometryContainment::Indeterminate;
        double lower=0,upper=0;
        ASSERT_EQ(GeometryDistanceStatus::Valid,
            body.encloseMinimumBoundaryDistance(points[i],1e-6,side,lower,upper)) << i;
        EXPECT_EQ(sides[i],side);
        EXPECT_LE(lower,expected[i]+1e-12); EXPECT_GE(upper,expected[i]-1e-12);
        EXPECT_LE(upper-lower,1e-6);
    }
    // The closest point to (1,0,0) is off axis; the radial direction alone
    // cannot produce a witness inside this narrow tolerance band.
    EXPECT_EQ(GeometryContainment::Boundary,
        body.classifyPointCertified(Vector3Dd(1,0,0),java::Math::sqrt(0.875)+1e-7));
    GeometryContainment side=GeometryContainment::Boundary;
    double lower=17,upper=23;
    EXPECT_EQ(GeometryDistanceStatus::InvalidInput,
        body.encloseMinimumBoundaryDistance(Vector3Dd(3.2,0,0),0,side,lower,upper));
    EXPECT_EQ(GeometryDistanceStatus::Indeterminate,
        body.encloseMinimumBoundaryDistance(Vector3Dd(3.2,0,0),1e-30,side,lower,upper));
    EXPECT_EQ(GeometryContainment::Boundary,side); EXPECT_DOUBLE_EQ(17,lower); EXPECT_DOUBLE_EQ(23,upper);
}

TEST(EllipsoidMinimumDistanceContract, ShearedSceneMatchesIndependentReferences) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    const Matrix4x4d forward=Matrix4x4d::identityMatrix().withVal(0,0,-2).withVal(0,1,0.5).withVal(1,2,-0.3);
    const Matrix4x4d inverse=Matrix4x4d::identityMatrix().withVal(0,0,-0.5).withVal(0,1,0.25)
        .withVal(0,2,0.075).withVal(1,2,0.3);
    ASSERT_EQ(AffinePlacementStatus::Valid,body.setAffinePlacement(forward,inverse));
    const double samples[][4]={{0.12,0.27,0.31,0.484219147169112},
        {0.65,0.1,-0.2,0.437976391530554},{1.1,0.2,0.3,0.228896833955284},
        {0.1,-1.2,0.5,0.341598824284725},{0.01,0.01,0.01,0.828855732290258},
        {0,0,0,0.84234037196832}};
    for (int i=0;i<6;++i) {
        const Vector3Dd local(samples[i][0],samples[i][1],samples[i][2]);
        GeometryContainment side=GeometryContainment::Indeterminate;
        double lower=0,upper=0;
        ASSERT_EQ(GeometryDistanceStatus::Valid,body.encloseMinimumBoundaryDistance(
            forward.multiply(local),1e-5,side,lower,upper)) << i;
        EXPECT_LE(lower,samples[i][3]+1e-9); EXPECT_GE(upper,samples[i][3]-1e-9);
        EXPECT_LE(upper-lower,1e-5);
    }
}

TEST(EllipsoidMinimumDistanceContract, CertifiedBoundsSurvivePlacementAndRadius) {
    Sphere sphere(2);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    const Matrix4x4d rotation=Matrix4x4d::identityMatrix().withVal(0,0,0).withVal(0,1,-1)
        .withVal(1,0,1).withVal(1,1,0);
    const Matrix4x4d inverseRotation=Matrix4x4d::identityMatrix().withVal(0,0,0).withVal(0,1,1)
        .withVal(1,0,-1).withVal(1,1,0);
    const Matrix4x4d shape=Matrix4x4d::identityMatrix().withVal(0,0,-2).withVal(0,1,0.5).withVal(1,2,-0.3);
    const Matrix4x4d inverseShape=Matrix4x4d::identityMatrix().withVal(0,0,-0.5)
        .withVal(0,1,0.25).withVal(0,2,0.075).withVal(1,2,0.3);
    const double samples[][4]={{0.12,0.27,0.31,0.484219147169112},
        {0.1,-1.2,0.5,0.341598824284725},{0,0,0,0.84234037196832}};
    const double factors[]={0.01,1,100};
    for (int k=0;k<3;++k) {
        const double factor=factors[k];
        const Matrix4x4d forward=Matrix4x4d().translation(4,-3,2).multiply(rotation)
            .multiply(Matrix4x4d().scale(factor,factor,factor)).multiply(shape);
        const Matrix4x4d inverse=inverseShape.multiply(Matrix4x4d().scale(1/factor,1/factor,1/factor))
            .multiply(inverseRotation).multiply(Matrix4x4d().translation(-4,3,-2));
        ASSERT_EQ(AffinePlacementStatus::Valid,body.setAffinePlacement(forward,inverse));
        for (int i=0;i<3;++i) {
            const Vector3Dd local(samples[i][0]*2,samples[i][1]*2,samples[i][2]*2);
            GeometryContainment side=GeometryContainment::Indeterminate;
            double lower=0,upper=0;
            ASSERT_EQ(GeometryDistanceStatus::Valid,body.encloseMinimumBoundaryDistance(
                forward.multiply(local),1e-5*factor,side,lower,upper)) << k << ", " << i;
            const double expected=2*factor*samples[i][3];
            EXPECT_LE(lower,expected+1e-9*factor); EXPECT_GE(upper,expected-1e-9*factor);
            EXPECT_LE(upper-lower,1e-5*factor);
        }
    }
}

TEST(EllipsoidMinimumDistanceContract, FailureAndBoundaryDoNotInventResults) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    body.setScale(Vector3Dd(3,2,1));
    GeometryContainment side=GeometryContainment::Inside;
    double lower=17,upper=23;
    EXPECT_EQ(GeometryDistanceStatus::InvalidInput,body.encloseMinimumBoundaryDistance(
        Vector3Dd(),java::Double::NaN,side,lower,upper));
    EXPECT_EQ(GeometryContainment::Inside,side); EXPECT_DOUBLE_EQ(17,lower); EXPECT_DOUBLE_EQ(23,upper);
    ASSERT_EQ(GeometryDistanceStatus::Valid,body.encloseMinimumBoundaryDistance(
        Vector3Dd(3,0,0),1e-8,side,lower,upper));
    EXPECT_LE(lower,0); EXPECT_GE(upper,0); EXPECT_LE(upper,1e-8);
    EXPECT_EQ(GeometryContainment::Indeterminate,side);
    side=GeometryContainment::Boundary; lower=17; upper=23;
    const int oldMode=fegetround();
    const int changed=fesetround(FE_UPWARD);
    const GeometryDistanceStatus unsupported=body.encloseMinimumBoundaryDistance(
        Vector3Dd(3.2,0,0),1e-5,side,lower,upper);
    const int restored=fesetround(oldMode);
    EXPECT_EQ(0,changed); EXPECT_EQ(0,restored);
    EXPECT_EQ(GeometryDistanceStatus::Unsupported,unsupported);
    EXPECT_EQ(GeometryContainment::Boundary,side); EXPECT_DOUBLE_EQ(17,lower); EXPECT_DOUBLE_EQ(23,upper);
    body.setScale(Vector3Dd(0,1,1));
    EXPECT_EQ(GeometryDistanceStatus::InvalidInput,body.encloseMinimumBoundaryDistance(
        Vector3Dd(),1e-5,side,lower,upper));
    EXPECT_EQ(GeometryContainment::Boundary,side); EXPECT_DOUBLE_EQ(17,lower); EXPECT_DOUBLE_EQ(23,upper);
}

TEST(EllipsoidMinimumDistanceContract, WrongInverseHintCannotFalsifyBounds) {
    GeometryContainment side=GeometryContainment::Indeterminate;
    double lower=-1,upper=-1;
    // The inverse hint is deliberately wrong. It may worsen the witness, but
    // the returned enclosure is calculated from the forward matrix itself.
    ASSERT_EQ(GeometryDistanceStatus::Valid,AffineEllipsoidMinimumDistance::query(
        Vector3Dd(3,0,0),1,Matrix4x4d::identityMatrix(),Matrix4x4d().scale(-1,1,1),
        10,side,lower,upper));
    EXPECT_EQ(GeometryContainment::Outside,side);
    EXPECT_LE(lower,2); EXPECT_GE(upper,2); EXPECT_LE(upper-lower,10);
    side=GeometryContainment::Boundary; lower=17; upper=23;
    EXPECT_EQ(GeometryDistanceStatus::Indeterminate,AffineEllipsoidMinimumDistance::query(
        Vector3Dd(3,0,0),1,Matrix4x4d::identityMatrix(),Matrix4x4d().scale(-1,1,1),
        1e-6,side,lower,upper));
    EXPECT_EQ(GeometryContainment::Boundary,side); EXPECT_DOUBLE_EQ(17,lower); EXPECT_DOUBLE_EQ(23,upper);
}

TEST(FloatingIntervalContract, IntrinsicSphereTolerancePreservesUncertainty) {
    Sphere sphere(3);
    EXPECT_EQ(GeometryContainment::Inside,sphere.classifyPoint(Vector3Dd(2.8,0,0),0.1));
    EXPECT_EQ(GeometryContainment::Boundary,sphere.classifyPoint(Vector3Dd(3.05,0,0),0.1));
    EXPECT_EQ(GeometryContainment::Outside,sphere.classifyPoint(Vector3Dd(3.2,0,0),0.1));
    EXPECT_EQ(GeometryContainment::Indeterminate,sphere.classifyPoint(Vector3Dd(3.1,0,0),0.1));
    EXPECT_THROW(sphere.doContainmentTest(Vector3Dd(3.1,0,0),0.1),java::IllegalStateException);
    EXPECT_EQ(GeometryContainment::Boundary,sphere.classifyPoint(Vector3Dd(3,0,0),0));
}

TEST(EllipsoidDistanceContract, OrthogonalAxesCoverExteriorInteriorAndHardCase) {
    const Matrix4x4d forward=Matrix4x4d().translation(5,-2,3).multiply(Matrix4x4d().scale(-3,2,1));
    double distance=-1, error=-1;
    ASSERT_EQ(GeometryDistanceStatus::Valid,
        OrthogonalEllipsoidDistance::query(Vector3Dd(5,-2,3),1,forward,distance,error));
    EXPECT_NEAR(1,distance,1e-10);
    ASSERT_EQ(GeometryDistanceStatus::Valid,
        OrthogonalEllipsoidDistance::query(Vector3Dd(1.8,-2,3),1,forward,distance,error));
    EXPECT_NEAR(0.2,distance,1e-10);
    ASSERT_EQ(GeometryDistanceStatus::Valid,
        OrthogonalEllipsoidDistance::query(Vector3Dd(4,-2,3),1,forward,distance,error));
    EXPECT_NEAR(java::Math::sqrt(0.875),distance,1e-10);
    EXPECT_GT(error,0);
    const Matrix4x4d shear=Matrix4x4d::identityMatrix().withVal(0,1,0.5);
    EXPECT_EQ(GeometryDistanceStatus::Unsupported,
        OrthogonalEllipsoidDistance::query(Vector3Dd(),1,shear,distance,error));
    EXPECT_EQ(GeometryDistanceStatus::InvalidInput,
        OrthogonalEllipsoidDistance::query(Vector3Dd(),0,forward,distance,error));
    EXPECT_EQ(GeometryDistanceStatus::Indeterminate,
        OrthogonalEllipsoidDistance::query(Vector3Dd(),1,Matrix4x4d().scale(1e-9,1,1),distance,error));
    // A nonzero column whose squared norm underflows is ill-conditioned,
    // not a proven singular input.
    distance=17; error=23;
    EXPECT_EQ(GeometryDistanceStatus::Indeterminate,
        OrthogonalEllipsoidDistance::query(Vector3Dd(),1,Matrix4x4d().scale(1e-300,1,1),distance,error));
    EXPECT_DOUBLE_EQ(17,distance); EXPECT_DOUBLE_EQ(23,error);
}

TEST(EllipsoidDistanceContract, ObliqueNormalOffsetsGiveWorldDistance) {
    const Matrix4x4d forward=Matrix4x4d().scale(3,2,1);
    const Vector3Dd surface(1.8,1.6,0);
    const Vector3Dd normal=Vector3Dd(1,2,0).multiply(1/java::Math::sqrt(5.0));
    double distance=0,error=0;
    ASSERT_EQ(GeometryDistanceStatus::Valid,
        OrthogonalEllipsoidDistance::query(surface.add(normal.multiply(0.2)),1,forward,distance,error));
    EXPECT_NEAR(0.2,distance,1e-9);
    ASSERT_EQ(GeometryDistanceStatus::Valid,
        OrthogonalEllipsoidDistance::query(surface.subtract(normal.multiply(0.05)),1,forward,distance,error));
    EXPECT_NEAR(0.05,distance,1e-9);
}

TEST(EllipsoidDistanceContract, DistancesAgreeWithIndependentSurfaceSampling) {
    const Matrix4x4d rotation=Matrix4x4d::identityMatrix().withVal(0,0,0).withVal(0,1,-1)
        .withVal(1,0,1).withVal(1,1,0);
    const Matrix4x4d forward=Matrix4x4d().translation(4,-3,2).multiply(rotation)
        .multiply(Matrix4x4d().scale(-3,2,1));
    const Vector3Dd localPoints[]={Vector3Dd(),Vector3Dd(0.1,0.3,0.2),Vector3Dd(-0.7,0.1,0),
        Vector3Dd(0.6,0.7,0.1),Vector3Dd(1.4,0.3,0.5),Vector3Dd(-0.5,-1.2,0.3)};
    for (int index=0;index<6;++index) {
        const Vector3Dd point=forward.multiply(localPoints[index]);
        double sampled=java::Double::MAX_VALUE;
        for (int latitude=0;latitude<=180;++latitude) {
            const double theta=java::Math::PI*latitude/180;
            for (int longitude=0;longitude<360;++longitude) {
                const double phi=2*java::Math::PI*longitude/360;
                const Vector3Dd unit(java::Math::sin(theta)*java::Math::cos(phi),
                    java::Math::sin(theta)*java::Math::sin(phi),java::Math::cos(theta));
                const double candidate=forward.multiply(unit).subtract(point).length();
                if (candidate<sampled) sampled=candidate;
            }
        }
        double distance=0,error=0;
        ASSERT_EQ(GeometryDistanceStatus::Valid,
            OrthogonalEllipsoidDistance::query(point,1,forward,distance,error));
        // The Frobenius norm is below 3.75; the two angular half-steps
        // sum to pi/180, so 0.07 bounds the surface sampling error.
        EXPECT_LE(distance,sampled+error);
        EXPECT_GE(distance,sampled-0.07-error);
    }
}

TEST(EllipsoidDistanceContract, RepeatedAxesRotationRadiusAndFailureAreExplicit) {
    const Matrix4x4d rotation=Matrix4x4d::identityMatrix().withVal(0,0,0).withVal(0,1,-1)
        .withVal(1,0,1).withVal(1,1,0);
    const Matrix4x4d forward=Matrix4x4d().translation(4,2,-3).multiply(rotation)
        .multiply(Matrix4x4d().scale(-2,2,1));
    double distance=-1,error=-1;
    ASSERT_EQ(GeometryDistanceStatus::Valid,
        OrthogonalEllipsoidDistance::query(Vector3Dd(4,2,-3),2,forward,distance,error));
    EXPECT_NEAR(2,distance,1e-10);
    const Vector3Dd endpoint=forward.multiply(Vector3Dd(2,0,0));
    ASSERT_EQ(GeometryDistanceStatus::Valid,
        OrthogonalEllipsoidDistance::query(endpoint,2,forward,distance,error));
    EXPECT_NEAR(0,distance,1e-10);
    distance=17; error=23;
    EXPECT_EQ(GeometryDistanceStatus::Unsupported,
        OrthogonalEllipsoidDistance::query(Vector3Dd(),2,
            Matrix4x4d::identityMatrix().withVal(0,1,0.3),distance,error));
    EXPECT_DOUBLE_EQ(17,distance);
    EXPECT_DOUBLE_EQ(23,error);
    EXPECT_EQ(GeometryDistanceStatus::InvalidInput,
        OrthogonalEllipsoidDistance::query(Vector3Dd(java::Double::NaN,0,0),2,forward,distance,error));
    EXPECT_EQ(GeometryDistanceStatus::InvalidInput,
        OrthogonalEllipsoidDistance::query(Vector3Dd(),2,Matrix4x4d().scale(0,1,1),distance,error));
    EXPECT_DOUBLE_EQ(17,distance);
    EXPECT_DOUBLE_EQ(23,error);
}

TEST(EllipsoidDistanceContract, ShearSupportsExactNormalOffsetsAndCenter) {
    const Matrix4x4d forward=Matrix4x4d().translation(3,-2,1).multiply(
        Matrix4x4d::identityMatrix().withVal(0,0,2).withVal(0,1,0.5));
    const Vector3Dd surface=forward.multiply(Vector3Dd(0.6,0.8,0));
    const Vector3Dd n(0.3,0.65,0);
    const Vector3Dd normal=n.multiply(1/n.length());
    double distance=0,error=0;
    ASSERT_EQ(GeometryDistanceStatus::Valid,
        AffineEllipsoidDistance::query(surface.add(normal.multiply(0.2)),1,forward,distance,error));
    EXPECT_NEAR(0.2,distance,1e-9);
    ASSERT_EQ(GeometryDistanceStatus::Valid,
        AffineEllipsoidDistance::query(surface.subtract(normal.multiply(0.05)),1,forward,distance,error));
    EXPECT_NEAR(0.05,distance,1e-9);
    ASSERT_EQ(GeometryDistanceStatus::Valid,
        AffineEllipsoidDistance::query(Vector3Dd(3,-2,1),1,forward,distance,error));
    EXPECT_NEAR(java::Math::sqrt((5.25-java::Math::sqrt(11.5625))/2),distance,1e-9);
    Vector3Dd witness;
    ASSERT_EQ(GeometryDistanceStatus::Valid,AffineEllipsoidDistance::queryWithWitness(
        surface.add(normal.multiply(0.2)),1,forward,distance,error,witness));
    EXPECT_NEAR(0,witness.subtract(surface).length(),1e-9);
}

TEST(EllipsoidDistanceContract, ShearedNearSingularityDoesNotInventDistance) {
    const Matrix4x4d forward=Matrix4x4d::identityMatrix().withVal(0,1,1).withVal(1,1,1e-8);
    double distance=17,error=23;
    EXPECT_EQ(GeometryDistanceStatus::Indeterminate,
        AffineEllipsoidDistance::query(Vector3Dd(),1,forward,distance,error));
    EXPECT_DOUBLE_EQ(17,distance);
    EXPECT_DOUBLE_EQ(23,error);
}

TEST(EllipsoidDistanceContract, SceneBodyExposesAffineDistanceAndFailures) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    const Matrix4x4d forward=Matrix4x4d().translation(3,-2,1).multiply(
        Matrix4x4d::identityMatrix().withVal(0,0,2).withVal(0,1,0.5));
    const Matrix4x4d inverse=Matrix4x4d::identityMatrix().withVal(0,0,0.5).withVal(0,1,-0.25)
        .multiply(Matrix4x4d().translation(-3,2,-1));
    ASSERT_EQ(AffinePlacementStatus::Valid,body.setAffinePlacement(forward,inverse));
    const Vector3Dd n(0.3,0.65,0);
    const Vector3Dd point=forward.multiply(Vector3Dd(0.6,0.8,0)).add(n.multiply(0.2/n.length()));
    double distance=0,error=0;
    ASSERT_EQ(GeometryDistanceStatus::Valid,body.estimateBoundaryDistance(point,distance,error));
    EXPECT_NEAR(0.2,distance,1e-9);
    EXPECT_GT(error,0);
    Box box(1,1,1);
    body.setGeometryReference(&box);
    distance=17; error=23;
    EXPECT_EQ(GeometryDistanceStatus::Unsupported,body.estimateBoundaryDistance(point,distance,error));
    EXPECT_DOUBLE_EQ(17,distance); EXPECT_DOUBLE_EQ(23,error);
    EXPECT_EQ(GeometryDistanceStatus::InvalidInput,
        body.estimateBoundaryDistance(Vector3Dd(java::Double::NaN,0,0),distance,error));
    body.setGeometryReference(&sphere);
    body.resetAffinePlacement();
    body.setScale(Vector3Dd(0,1,1));
    EXPECT_EQ(GeometryDistanceStatus::InvalidInput,body.estimateBoundaryDistance(point,distance,error));
    body.setScale(Vector3Dd(1e-12,1,1));
    EXPECT_EQ(GeometryDistanceStatus::Indeterminate,body.estimateBoundaryDistance(point,distance,error));
    EXPECT_DOUBLE_EQ(17,distance); EXPECT_DOUBLE_EQ(23,error);
}

TEST(EllipsoidDistanceContract, ShearedDistanceAgreesWithIndependentSurfaceSampling) {
    const Matrix4x4d forward=Matrix4x4d().translation(1,-3,2).multiply(
        Matrix4x4d::identityMatrix().withVal(0,0,-2).withVal(0,1,0.5).withVal(1,2,-0.3));
    const Vector3Dd samples[]={Vector3Dd(),Vector3Dd(0.1,0.3,0.2),Vector3Dd(-0.7,0.1,0),
        Vector3Dd(0.6,0.7,0.1),Vector3Dd(1.4,0.3,0.5),Vector3Dd(-0.5,-1.2,0.3)};
    for (int index=0;index<6;++index) {
        const Vector3Dd point=forward.multiply(samples[index]);
        double sampled=java::Double::MAX_VALUE;
        for (int latitude=0;latitude<=180;++latitude) {
            const double theta=java::Math::PI*latitude/180;
            for (int longitude=0;longitude<360;++longitude) {
                const double phi=2*java::Math::PI*longitude/360;
                const Vector3Dd unit(java::Math::sin(theta)*java::Math::cos(phi),
                    java::Math::sin(theta)*java::Math::sin(phi),java::Math::cos(theta));
                const double candidate=forward.multiply(unit).subtract(point).length();
                if (candidate<sampled) sampled=candidate;
            }
        }
        double distance=0,error=0;
        ASSERT_EQ(GeometryDistanceStatus::Valid,
            AffineEllipsoidDistance::query(point,1,forward,distance,error));
        // ||A||_F < 2.6 and the sum of angular half-steps is pi/180.
        EXPECT_LE(distance,sampled+error);
        EXPECT_GE(distance,sampled-0.05-error);
    }
}

TEST(EllipsoidDistanceContract, ShearedDistanceMatchesIndependentConstrainedMinimization) {
    const Matrix4x4d forward=Matrix4x4d::identityMatrix().withVal(0,0,-2)
        .withVal(0,1,0.5).withVal(1,2,-0.3);
    // Generated by generate_ellipsoid_distance_reference.py: independent
    // constrained SLSQP with eight surface seeds, keeping the smallest value.
    const double samples[][4]={
        {0.12,0.27,0.31,0.484219147169112},{0.65,0.1,-0.2,0.437976391530554},
        {0.21,-0.72,0.35,0.200937270429387},{0.35,0.55,0.15,0.293307568130137},
        {1.1,0.2,0.3,0.228896833955284},{-1.1,0.8,0.4,0.717026711154501},
        {0.1,-1.2,0.5,0.341598824284725},{0.3,0.2,1.5,0.529526975338513},
        {-0.9,-0.4,-0.3,0.0336629761551959},{0.01,0.01,0.01,0.828855732290258},
        {0,0,0,0.84234037196832},{1.5,-1.4,0.9,2.19701576003708}
    };
    for (int i=0;i<12;++i) {
        const Vector3Dd point=forward.multiply(Vector3Dd(samples[i][0],samples[i][1],samples[i][2]));
        double distance=0,error=0;
        ASSERT_EQ(GeometryDistanceStatus::Valid,
            AffineEllipsoidDistance::query(point,1,forward,distance,error)) << i;
        EXPECT_NEAR(samples[i][3],distance,5e-7) << i;
        EXPECT_LE(java::Math::abs(samples[i][3]-distance),error+5e-12) << i;
    }
}

TEST(EllipsoidDistanceContract, NumericalReferenceSurvivesRotationTranslationAndScale) {
    const Matrix4x4d rotation=Matrix4x4d::identityMatrix().withVal(0,0,0).withVal(0,1,-1)
        .withVal(1,0,1).withVal(1,1,0);
    const Matrix4x4d shape=Matrix4x4d::identityMatrix().withVal(0,0,-2)
        .withVal(0,1,0.5).withVal(1,2,-0.3);
    const double samples[][4]={{0.12,0.27,0.31,0.484219147169112},
        {0.1,-1.2,0.5,0.341598824284725},{0.01,0.01,0.01,0.828855732290258},
        {-0.9,-0.4,-0.3,0.0336629761551959}};
    const double factors[]={0.01,1,100};
    for (int k=0;k<3;++k) {
        const double factor=factors[k];
        const Matrix4x4d forward=Matrix4x4d().translation(4,-3,2).multiply(rotation)
            .multiply(Matrix4x4d().scale(factor,factor,factor)).multiply(shape);
        for (int i=0;i<4;++i) {
            const Vector3Dd point=forward.multiply(Vector3Dd(samples[i][0],samples[i][1],samples[i][2]));
            double distance=0,error=0;
            ASSERT_EQ(GeometryDistanceStatus::Valid,
                AffineEllipsoidDistance::query(point,1,forward,distance,error)) << k << ", " << i;
            EXPECT_NEAR(samples[i][3]*factor,distance,5e-7*factor) << k << ", " << i;
        }
    }
}

TEST(RayAffineContract, CopiesPreserveUnchangedComponents) {
    const Ray original = Ray::fromAffine(Vector3Dd(-3,0,0), Vector3Dd(2,0,0), 1);
    const Ray atExit = original.withT(2);
    EXPECT_DOUBLE_EQ(2, atExit.getDirection().x());
    EXPECT_DOUBLE_EQ(1, atExit.getOrigin().add(atExit.getDirection().multiply(atExit.getT())).x());
    EXPECT_DOUBLE_EQ(1, original.getT());
    const Ray shifted = original.withOrigin(Vector3Dd(-5,0,0));
    EXPECT_DOUBLE_EQ(2, shifted.getDirection().x());
    EXPECT_DOUBLE_EQ(1, shifted.getT());
    const Ray redirected = original.withDirection(Vector3Dd(4,1,0));
    EXPECT_DOUBLE_EQ(4, redirected.getDirection().x());
    EXPECT_DOUBLE_EQ(1, redirected.getDirection().y());
    EXPECT_DOUBLE_EQ(-3, redirected.getOrigin().x());
    EXPECT_DOUBLE_EQ(1, redirected.getT());
    const Ray copied = Ray::copyOf(original);
    EXPECT_TRUE(copied.equals(original));
    EXPECT_EQ(0, java::Double::compare(original.withT(-0.0).getT(), -0.0));
}

TEST(RayAffineContract, ExplicitAffineConstructionPreservesLegacyConstructor) {
    Ray legacy(Vector3Dd(), Vector3Dd(2,0,0));
    EXPECT_DOUBLE_EQ(1, legacy.getDirection().x());
    Ray affine = Ray::fromAffine(Vector3Dd(), Vector3Dd(2,0,0));
    EXPECT_DOUBLE_EQ(2, affine.getDirection().x());
    EXPECT_DOUBLE_EQ(0, affine.getT());
}

TEST(TriangleContract, SharedKernelPreservesAffineRayAndBarycentrics) {
    const Vector3Dd a(0,0,0), b(2,0,0), c(0,2,0);
    const Ray ray = Ray::fromAffine(Vector3Dd(0.5,0.5,4), Vector3Dd(0,0,-2));
    double t = 0, u = 0, v = 0;
    ASSERT_TRUE(Triangle::intersectRay(ray, a, b, c, t, u, v));
    EXPECT_DOUBLE_EQ(2, t);
    EXPECT_DOUBLE_EQ(0.25, u);
    EXPECT_DOUBLE_EQ(0.25, v);
    Intersection* hit = Triangle::doIntersectionWithTriangle(ray, a, b, c);
    ASSERT_NE(nullptr, hit);
    EXPECT_DOUBLE_EQ(t, hit->t);
    delete hit;
    EXPECT_TRUE(Triangle::intersectRay(ray, a, c, b, t, u, v));
    EXPECT_FALSE(Triangle::intersectRay(Ray::fromAffine(Vector3Dd(3,3,1), Vector3Dd(0,0,-1)),
        a, b, c, t, u, v));
    EXPECT_FALSE(Triangle::intersectRay(Ray::fromAffine(Vector3Dd(0.5,0.5,1), Vector3Dd(1,0,0)),
        a, b, c, t, u, v));
    EXPECT_FALSE(Triangle::intersectRay(ray, a, a, c, t, u, v));
}

TEST(TriangleContract, ProjectedContainmentAndWindingWorkOnEveryAxis) {
    const Vector3Dd first(0,0,0);
    const Vector3Dd second[] = {Vector3Dd(0,2,0), Vector3Dd(2,0,0), Vector3Dd(2,0,0)};
    const Vector3Dd third[] = {Vector3Dd(0,0,2), Vector3Dd(0,0,2), Vector3Dd(0,2,0)};
    const Vector3Dd inside[] = {Vector3Dd(0,0.5,0.5), Vector3Dd(0.5,0,0.5), Vector3Dd(0.5,0.5,0)};
    const Vector3Dd outside[] = {Vector3Dd(0,2,2), Vector3Dd(2,0,2), Vector3Dd(2,2,0)};
    for (int i = 0; i < 3; ++i) {
        for (int reversed = 0; reversed < 2; ++reversed) {
            Vector3Dd a = first, b = reversed ? third[i] : second[i];
            const Vector3Dd c = reversed ? second[i] : third[i];
            Vector3Dd normal;
            double distance = 0;
            ASSERT_TRUE(Triangle::supportingPlane(a, b, c, 1e-9, normal, distance));
            const Triangle::ProjectionAxis axis = Triangle::dominantProjection(normal);
            EXPECT_EQ(i, static_cast<int>(axis));
            if (Triangle::requiresProjectionSwap(a, b, c, axis)) {
                const Vector3Dd temporary = a; a = b; b = temporary;
            }
            EXPECT_TRUE(Triangle::containsProjected(a, b, c, inside[i], axis));
            EXPECT_FALSE(Triangle::containsProjected(a, b, c, outside[i], axis));
            EXPECT_TRUE(Triangle::containsProjected(a, b, c, a, axis));
        }
    }
    Vector3Dd normal;
    double distance = 0;
    EXPECT_FALSE(Triangle::supportingPlane(first, first, first, 1e-9, normal, distance));
    EXPECT_FALSE(Triangle::supportingPlane(first, first, first, 0, normal, distance));
}

TEST(SphereFirstHitContract, FirstHitCrossingsAndDetailUseSameParameter) {
    Sphere sphere(1);
    Ray ray = Ray::fromAffine(Vector3Dd(-3,0,0), Vector3Dd(2,0,0));
    RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL);
    ASSERT_TRUE(sphere.doIntersectionFirstHit(ray, &hit));
    EXPECT_DOUBLE_EQ(1, hit.getHitDistance());
    EXPECT_DOUBLE_EQ(-1, hit.point.x());
    EXPECT_DOUBLE_EQ(-1, hit.normal.x());
    ASSERT_NE(nullptr, hit.getRay());
    EXPECT_DOUBLE_EQ(2, hit.getRay()->getDirection().x());
    RayCrossingTestCollector crossings;
    sphere.emitRayCrossings(ray, RayCrossingQuery(0,10), crossings);
    ASSERT_EQ(2L, crossings.roots.size());
    EXPECT_DOUBLE_EQ(crossings.roots.get(0), hit.getHitDistance());
    Ray* pointerHit = sphere.doIntersectionFirstHit(ray);
    ASSERT_NE(nullptr, pointerHit);
    EXPECT_DOUBLE_EQ(hit.getHitDistance(), pointerHit->getT());
    EXPECT_DOUBLE_EQ(2, pointerHit->getDirection().x());
    delete pointerHit;
}

TEST(SphereFirstHitContract, InteriorExitAndClippedFarRoot) {
    Sphere sphere(1);
    Ray ray = Ray::fromAffine(Vector3Dd(), Vector3Dd(2,0,0));
    RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL);
    ASSERT_TRUE(sphere.doIntersectionFirstHit(ray, &hit));
    EXPECT_DOUBLE_EQ(0.5, hit.getHitDistance());
    EXPECT_DOUBLE_EQ(1, hit.point.x());
    EXPECT_DOUBLE_EQ(1, hit.normal.x());
    ray = ray.withOrigin(Vector3Dd(-3,0,0));
    ASSERT_TRUE(sphere.doIntersectionFirstHit(ray, RayCrossingQuery(1.25,2), &hit));
    EXPECT_DOUBLE_EQ(2, hit.getHitDistance());
    EXPECT_DOUBLE_EQ(1, hit.point.x());
    EXPECT_FALSE(sphere.doIntersectionFirstHit(ray, RayCrossingQuery(1.25,1.75), &hit));
    EXPECT_TRUE(sphere.doIntersectionFirstHit(ray, RayCrossingQuery(2,2), nullptr));
    EXPECT_FALSE(sphere.doIntersectionFirstHit(ray, RayCrossingQuery(3,2), &hit));
}

TEST(SphereFirstHitContract, TangencyAndDistanceOnly) {
    Sphere sphere(1);
    Ray ray = Ray::fromAffine(Vector3Dd(-2,1,0), Vector3Dd(1,0,0));
    RayHit hit(RayHit::DETAIL_NONE, false);
    ASSERT_TRUE(sphere.doIntersectionFirstHit(ray, &hit));
    EXPECT_DOUBLE_EQ(2, hit.getHitDistance());
    EXPECT_EQ(nullptr, hit.getRay());
    ray.setDirection(Vector3Dd());
    EXPECT_FALSE(sphere.doIntersectionFirstHit(ray, &hit));
    Sphere oblique(java::Math::sqrt(5.0));
    ray = Ray::fromAffine(Vector3Dd(-6,-3,0), Vector3Dd(2,1,0));
    ASSERT_TRUE(oblique.doIntersectionFirstHit(ray, &hit));
    EXPECT_NEAR(2, hit.getHitDistance(), 1e-12);
}

TEST(SphereFirstHitContract, SceneFastPathAndDetailAgreeForAffineRay) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    body.setPosition(Vector3Dd(10,0,0));
    Ray ray = Ray::fromAffine(Vector3Dd(7,0,0), Vector3Dd(2,0,0));
    RayHit distance(RayHit::DETAIL_NONE, false);
    RayHit detail(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL);
    ASSERT_TRUE(body.doIntersectionFirstHit(ray, &distance));
    ASSERT_TRUE(body.doIntersectionFirstHit(ray, &detail));
    EXPECT_DOUBLE_EQ(1, distance.getHitDistance());
    EXPECT_DOUBLE_EQ(distance.getHitDistance(), detail.getHitDistance());
    EXPECT_DOUBLE_EQ(9, detail.point.x());
    EXPECT_DOUBLE_EQ(-1, detail.normal.x());
    ASSERT_NE(nullptr, detail.getRay());
    EXPECT_DOUBLE_EQ(2, detail.getRay()->getDirection().x());
    ray = ray.withOrigin(Vector3Dd(10,0,0));
    ASSERT_TRUE(body.doIntersectionFirstHit(ray, &distance));
    ASSERT_TRUE(body.doIntersectionFirstHit(ray, &detail));
    EXPECT_DOUBLE_EQ(0.5, distance.getHitDistance());
    EXPECT_DOUBLE_EQ(distance.getHitDistance(), detail.getHitDistance());
    EXPECT_DOUBLE_EQ(11, detail.point.x());
}

TEST(SphereFirstHitContract, NegativeNonuniformScalePreservesWorldParameter) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    body.setPosition(Vector3Dd(10,0,0));
    body.setScale(Vector3Dd(-2,3,1));
    Ray ray = Ray::fromAffine(Vector3Dd(4,0,0), Vector3Dd(2,0,0));
    RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL);
    ASSERT_TRUE(body.doIntersectionFirstHit(ray, &hit));
    EXPECT_DOUBLE_EQ(2, hit.getHitDistance());
    EXPECT_DOUBLE_EQ(8, hit.point.x());
    EXPECT_DOUBLE_EQ(-1, hit.normal.x());
    ASSERT_NE(nullptr, hit.getRay());
    EXPECT_DOUBLE_EQ(2, hit.getRay()->getDirection().x());
    ray = ray.withOrigin(Vector3Dd(10,0,0));
    ASSERT_TRUE(body.doIntersectionFirstHit(ray, &hit));
    EXPECT_DOUBLE_EQ(1, hit.getHitDistance());
    EXPECT_DOUBLE_EQ(12, hit.point.x());
    EXPECT_DOUBLE_EQ(1, hit.normal.x());
}

TEST(SphereFirstHitContract, InwardShadowBiasFindsTheAnalyticExit) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    const Vector3Dd surfacePoint(1,0,0);
    const Vector3Dd lightDirection = Vector3Dd(-0.1,1,0).normalized();
    const double bias = 1e-6;
    Ray shadowRay = Ray::fromAffine(surfacePoint.add(lightDirection.multiply(bias)),
        lightDirection);
    EXPECT_EQ(Geometry::INSIDE, sphere.doContainmentTest(shadowRay.getOrigin(), 0));
    RayHit hit(RayHit::DETAIL_NONE, false);
    ASSERT_TRUE(body.doIntersectionFirstHit(shadowRay, &hit));
    // On the unit sphere, the other chord endpoint lies at -2*(p dot d).
    const double expectedExit = -2 * surfacePoint.dotProduct(lightDirection) - bias;
    EXPECT_NEAR(expectedExit, hit.getHitDistance(), 1e-12);
    EXPECT_GT(hit.getHitDistance(), bias);
}


TEST(SphereDetailContract, RequestedAndAvailableAreDistinctAndResettable) {
    Sphere sphere(1);
    Ray ray = Ray::fromAffine(Vector3Dd(0,0,2), Vector3Dd(0,0,-1));
    RayHit hit(RayHit::DETAIL_ALL);
    EXPECT_FALSE(hit.isDetailAvailabilityKnown());
    ASSERT_TRUE(sphere.doIntersectionFirstHit(ray, &hit));
    EXPECT_TRUE(hit.isDetailAvailabilityKnown());
    EXPECT_EQ(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL, hit.getAvailableDetailMask());
    EXPECT_EQ(static_cast<int>(RayHit::DETAIL_ALL), hit.getRequiredDetailMask());
    RayHit copied(hit);
    EXPECT_TRUE(copied.isDetailAvailabilityKnown());
    EXPECT_EQ(hit.getAvailableDetailMask(), copied.getAvailableDetailMask());
    hit.resetForDistanceOnly();
    EXPECT_FALSE(hit.isDetailAvailabilityKnown());
    EXPECT_EQ(static_cast<int>(RayHit::DETAIL_NONE), hit.getAvailableDetailMask());
    EXPECT_TRUE(copied.isDetailAvailabilityKnown());
}

#include "vsdk/toolkit/environment/geometry/element/GeometryQueryWorkspace.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"

TEST(GeometryQueryContract, DistinguishesUnsupportedMissAndInvalidInput) {
    Sphere sphere(1);
    const Geometry* geometry = &sphere;
    Ray ray = Ray::fromAffine(Vector3Dd(-3,0,0), Vector3Dd(2,0,0));
    GeometryQueryWorkspace workspace;
    RayHit& hit = workspace.hitResult();
    hit.reset(RayHit::DETAIL_POINT);
    RayBoundary firstBoundary;
    EXPECT_EQ(RayFirstHitStatus::Hit, geometry->queryFirstHit(ray, RayCrossingQuery(1.25,2), &hit, &firstBoundary));
    EXPECT_EQ(&sphere, firstBoundary.geometry);
    EXPECT_DOUBLE_EQ(2, firstBoundary.t);
    EXPECT_FALSE(firstBoundary.normalReversed);
    EXPECT_DOUBLE_EQ(2, hit.getHitDistance());
    EXPECT_DOUBLE_EQ(1, hit.point.x());
    EXPECT_TRUE(hit.isDetailAvailabilityKnown());
    EXPECT_EQ(static_cast<int>(RayHit::DETAIL_POINT), hit.getAvailableDetailMask());
    EXPECT_EQ(RayFirstHitStatus::Miss, geometry->queryFirstHit(ray, RayCrossingQuery(1.25,1.75), &hit, &firstBoundary));
    EXPECT_EQ(nullptr, firstBoundary.geometry);
    EXPECT_FALSE(hit.hasHitDistance());
    EXPECT_EQ(RayFirstHitStatus::InvalidInput, geometry->queryFirstHit(ray, RayCrossingQuery(2,1), &hit));
    ray.setDirection(Vector3Dd(0,0,0));
    EXPECT_EQ(RayFirstHitStatus::InvalidInput, geometry->queryFirstHit(ray, RayCrossingQuery(0,3), &hit));
    ray.setDirection(Vector3Dd(1e300,0,0));
    EXPECT_EQ(RayFirstHitStatus::Indeterminate, geometry->queryFirstHit(ray, RayCrossingQuery(0,3), &hit));
    Torus unsupported(2,1);
    EXPECT_EQ(RayFirstHitStatus::Unsupported, unsupported.queryFirstHit(ray, RayCrossingQuery(0,3), &hit));
}

TEST(GeometryQueryContract, WorkspaceAndExtensibleProvenanceAreIndependent) {
    Sphere sphere(1);
    SimpleBody instance;
    instance.setGeometryReference(&sphere);
    GeometryQueryWorkspace workspace;
    Ray ray = Ray::fromAffine(Vector3Dd(-3,0,0), Vector3Dd(2,0,0));
    ASSERT_EQ(RaySolidQueryStatus::Supported,
        sphere.classifyRay(ray, RayCrossingQuery(0,3), workspace.solidResult()));
    ASSERT_EQ(2L, workspace.solidResult().boundaries.size());
    RaySolidBoundary& boundary = workspace.solidResult().boundaries[0];
    EXPECT_EQ(&sphere, boundary.geometry);
    EXPECT_EQ(-1, boundary.getProvenance().getSurfaceIndex());
    ASSERT_TRUE(boundary.getProvenance().setSurfaceIndex(7));
    EXPECT_FALSE(boundary.getProvenance().setSurfaceIndex(-2));
    boundary.normalReversed = true;
    Matrix4x4d forward = Matrix4x4d().translation(1,2,3);
    Matrix4x4d inverse = Matrix4x4d().translation(-1,-2,-3);
    RayInstanceStep step(&instance, forward, inverse);
    for (int i = 0; i < 32; i++) ASSERT_TRUE(boundary.appendInstance(step));
    EXPECT_FALSE(boundary.appendInstance(RayInstanceStep()));
    EXPECT_EQ(32L, boundary.getInstanceCount());
    RaySolidBoundary copied(boundary);
    boundary.clearInstances();
    EXPECT_EQ(7, boundary.getProvenance().getSurfaceIndex());
    EXPECT_EQ(32L, copied.getInstanceCount());
    EXPECT_EQ(7, copied.getProvenance().getSurfaceIndex());
    EXPECT_TRUE(copied.normalReversed);
    RayInstanceStep recovered;
    ASSERT_TRUE(copied.getInstance(31, recovered));
    EXPECT_EQ(&instance, recovered.instance);
    EXPECT_DOUBLE_EQ(1, recovered.childToParent.multiply(Vector3Dd()).x());
    EXPECT_DOUBLE_EQ(0, recovered.parentToChild.multiply(recovered.childToParent.multiply(Vector3Dd())).x());
    EXPECT_FALSE(copied.getInstance(32, recovered));
    workspace.clear();
    EXPECT_TRUE(workspace.solidResult().boundaries.isEmpty());
    EXPECT_EQ(GeometryBounds::Kind::Unsupported, workspace.boundsResult().getKind());
    EXPECT_FALSE(workspace.hitResult().hasHitDistance());
    EXPECT_EQ(32L, copied.getInstanceCount());
}


TEST(GeometryQueryContract, SceneInstancePreservesAffineParameterAndProvenance) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    body.setPosition(Vector3Dd(10,0,0));
    body.setScale(Vector3Dd(-2,3,1));
    Ray ray = Ray::fromAffine(Vector3Dd(4,0,0), Vector3Dd(2,0,0));
    GeometryQueryWorkspace workspace;
    ASSERT_EQ(RaySolidQueryStatus::Supported, body.classifyRay(ray, RayCrossingQuery(0,5), workspace.solidResult()));
    ASSERT_EQ(2L, workspace.solidResult().boundaries.size());
    EXPECT_DOUBLE_EQ(2, workspace.solidResult().boundaries[0].t);
    EXPECT_DOUBLE_EQ(4, workspace.solidResult().boundaries[1].t);
    EXPECT_EQ(&sphere, workspace.solidResult().boundaries[0].geometry);
    RayInstanceStep step;
    ASSERT_TRUE(workspace.solidResult().boundaries[0].getInstance(0, step));
    EXPECT_EQ(&body, step.instance);
    const Vector3Dd worldPoint = ray.getOrigin().add(ray.getDirection().multiply(2));
    const Vector3Dd localPoint = step.parentToChild.multiply(worldPoint);
    EXPECT_DOUBLE_EQ(1, localPoint.x());
    EXPECT_DOUBLE_EQ(worldPoint.x(), step.childToParent.multiply(localPoint).x());
    body.setScale(Vector3Dd(0,3,1));
    EXPECT_EQ(RaySolidQueryStatus::InvalidInput, body.classifyRay(ray, RayCrossingQuery(0,5), workspace.solidResult()));
    EXPECT_TRUE(workspace.solidResult().boundaries.isEmpty());
}

TEST(BoxContract, InstancePreservesAffineParameterAndProvenance) {
    Box box(2,2,2);
    SimpleBody body;
    body.setGeometryReference(&box);
    body.setPosition(Vector3Dd(10,0,0));
    body.setScale(Vector3Dd(-2,3,1));
    const Ray ray = Ray::fromAffine(Vector3Dd(4,0,0), Vector3Dd(2,0,0));
    GeometryQueryWorkspace workspace;
    ASSERT_EQ(RaySolidQueryStatus::Supported,
        body.classifyRay(ray, RayCrossingQuery(0,5), workspace.solidResult()));
    ASSERT_EQ(2, workspace.solidResult().boundaries.size());
    EXPECT_DOUBLE_EQ(2, workspace.solidResult().boundaries[0].t);
    EXPECT_DOUBLE_EQ(4, workspace.solidResult().boundaries[1].t);
    EXPECT_EQ(&box, workspace.solidResult().boundaries[0].geometry);
    RayInstanceStep step;
    ASSERT_TRUE(workspace.solidResult().boundaries[0].getInstance(0, step));
    EXPECT_EQ(&body, step.instance);
    EXPECT_EQ(GeometryContainment::Indeterminate, body.classifyPoint(Vector3Dd(10,0,0), 0.1));
}

TEST(PlanarHalfSpaceContract, ReflectedInstanceKeepsBoundaryAndInterior) {
    PlanarHalfSpace half(1,0,0,-1);
    SimpleBody body;
    body.setGeometryReference(&half);
    body.setScale(Vector3Dd(-2,3,1));
    const Ray ray = Ray::fromAffine(Vector3Dd(-4,0,0), Vector3Dd(1,0,0));
    GeometryQueryWorkspace workspace;
    ASSERT_EQ(RaySolidQueryStatus::Supported,
        body.classifyRay(ray, RayCrossingQuery(0,5), workspace.solidResult()));
    ASSERT_EQ(1, workspace.solidResult().boundaries.size());
    EXPECT_DOUBLE_EQ(2, workspace.solidResult().boundaries[0].t);
    EXPECT_EQ(&half, workspace.solidResult().boundaries[0].geometry);
    ASSERT_EQ(2, workspace.solidResult().insideSegments.size());
    EXPECT_FALSE(workspace.solidResult().insideSegments[0]);
    EXPECT_TRUE(workspace.solidResult().insideSegments[1]);
    RayInstanceStep step;
    ASSERT_TRUE(workspace.solidResult().boundaries[0].getInstance(0, step));
    EXPECT_EQ(&body, step.instance);
}


TEST(GeometryQueryContract, PointContainmentSeparatesBoundaryAndUnsupported) {
    Sphere sphere(1);
    const Geometry* geometry = &sphere;
    EXPECT_EQ(GeometryContainment::Inside, geometry->classifyPoint(Vector3Dd(0,0,0), 0));
    EXPECT_EQ(GeometryContainment::Boundary, geometry->classifyPoint(Vector3Dd(1,0,0), 0));
    EXPECT_EQ(GeometryContainment::Boundary, geometry->classifyPoint(Vector3Dd(1.01,0,0), 0.02));
    EXPECT_EQ(GeometryContainment::Outside, geometry->classifyPoint(Vector3Dd(2,0,0), 0));
    EXPECT_EQ(GeometryContainment::InvalidInput, geometry->classifyPoint(Vector3Dd(), -1));
    EXPECT_EQ(GeometryContainment::InvalidInput, geometry->classifyPoint(Vector3Dd(), java::Double::NaN));
    EXPECT_EQ(GeometryContainment::Indeterminate, geometry->classifyPoint(Vector3Dd(1e300,0,0), 0));
    Sphere tiny(1e-300);
    EXPECT_EQ(GeometryContainment::Indeterminate, tiny.classifyPoint(Vector3Dd(2e-300,0,0), 0));
    Torus unsupported(2,1);
    EXPECT_EQ(GeometryContainment::Unsupported, unsupported.classifyPoint(Vector3Dd(), 0));
}

TEST(GeometryQueryContract, CrossingFailuresAreNotReportedAsMisses) {
    Sphere sphere(1);
    RayCrossingTestCollector crossings;
    Ray ray = Ray::fromAffine(Vector3Dd(-3,0,0), Vector3Dd(0,0,0));
    EXPECT_EQ(RayCrossingSupport::InvalidInput, sphere.emitRayCrossings(ray, RayCrossingQuery(0,3), crossings));
    ray.setDirection(Vector3Dd(1e300,0,0));
    EXPECT_EQ(RayCrossingSupport::Indeterminate, sphere.emitRayCrossings(ray, RayCrossingQuery(0,3), crossings));
    EXPECT_TRUE(crossings.roots.isEmpty());
}

TEST(GeometryQueryContract, ShearRoundTripPreservesTheRayParameter) {
    Sphere sphere(1);
    const Matrix4x4d shear = Matrix4x4d::identityMatrix().withVal(0,1,0.5);
    const Matrix4x4d inverse = Matrix4x4d::identityMatrix().withVal(0,1,-0.5);
    const Ray local = Ray::fromAffine(Vector3Dd(-3,0.5,0), Vector3Dd(2,0,0));
    const Ray world = Ray::fromAffine(shear.multiply(local.getOrigin()),
        shear.withoutTranslation().multiply(local.getDirection()));
    const Ray recovered = Ray::fromAffine(inverse.multiply(world.getOrigin()),
        inverse.withoutTranslation().multiply(world.getDirection()));
    RayHit localHit(RayHit::DETAIL_POINT, false);
    RayHit recoveredHit(RayHit::DETAIL_POINT, false);
    ASSERT_EQ(RayFirstHitStatus::Hit, sphere.queryFirstHit(local, RayCrossingQuery(0,4), &localHit));
    ASSERT_EQ(RayFirstHitStatus::Hit, sphere.queryFirstHit(recovered, RayCrossingQuery(0,4), &recoveredHit));
    EXPECT_DOUBLE_EQ(localHit.getHitDistance(), recoveredHit.getHitDistance());
    const Vector3Dd expected = shear.multiply(localHit.point);
    const Vector3Dd worldAtT = world.getOrigin().add(world.getDirection().multiply(recoveredHit.getHitDistance()));
    EXPECT_DOUBLE_EQ(expected.x(), worldAtT.x());
    EXPECT_DOUBLE_EQ(expected.y(), worldAtT.y());
}


TEST(GeometryQueryContract, SurfaceEventsUseTheSameExtensibleProvenanceModel) {
    Sphere sphere(1);
    SimpleBody body;
    RayCrossing event(1, &sphere, RayCrossingKind::Entering);
    RayInstanceStep step(&body, Matrix4x4d(), Matrix4x4d());
    ASSERT_TRUE(event.getProvenance().setSurfaceIndex(3));
    ASSERT_TRUE(event.getProvenance().appendInstance(step));
    event.normalReversed = true;
    RayCrossing copy(event);
    EXPECT_TRUE(copy.normalReversed);
    event.getProvenance().clear();
    EXPECT_EQ(-1, event.getProvenance().getSurfaceIndex());
    EXPECT_EQ(3, copy.getProvenance().getSurfaceIndex());
    EXPECT_EQ(1L, copy.getProvenance().getInstanceCount());
    RayInstanceStep restored;
    ASSERT_TRUE(copy.getProvenance().getInstance(0, restored));
    EXPECT_EQ(&body, restored.instance);
}


#include "java/lang/Thread.h"
#include "GeometryWorkspaceTestWorker.h"
TEST(GeometryQueryContract, SeparateWorkerWorkspacesCanRunConcurrently) {
    Sphere sphere(1);
    GeometryWorkspaceTestWorker first(&sphere), second(&sphere);
    java::Thread firstThread(&first), secondThread(&second);
    firstThread.start(); secondThread.start();
    firstThread.join(); secondThread.join();
    EXPECT_TRUE(first.passed());
    EXPECT_TRUE(second.passed());
    EXPECT_NE(nullptr, first.boundaryStorage());
    EXPECT_NE(nullptr, second.boundaryStorage());
    EXPECT_NE(first.boundaryStorage(), second.boundaryStorage());
}


TEST(GeometryQueryContract, NestedWorkspaceScratchIsStableAndRequiresLifoRelease) {
    GeometryQueryWorkspace workspace;
    java::ArrayList<RaySolidClassification*> acquired;
    for (int i = 0; i < 32; i++) {
        RaySolidClassification* slot = workspace.acquireSolidScratch();
        ASSERT_NE(nullptr, slot);
        slot->insideSegments.add(true);
        acquired.add(slot);
    }
    EXPECT_FALSE(workspace.releaseSolidScratch(acquired[0]));
    EXPECT_TRUE(acquired[0]->insideSegments.get(0));
    for (int i = 31; i >= 0; i--) ASSERT_TRUE(workspace.releaseSolidScratch(acquired[i]));
    RaySolidClassification* reused = workspace.acquireSolidScratch();
    EXPECT_EQ(acquired[0], reused);
    EXPECT_TRUE(reused->insideSegments.isEmpty());
    workspace.clear();
    EXPECT_FALSE(workspace.releaseSolidScratch(reused));
    EXPECT_EQ(reused, workspace.acquireSolidScratch());
}


TEST(GeometryQueryContract, SceneContainmentHasExplicitWorldTolerancePolicy) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    body.setPosition(Vector3Dd(10,0,0));
    body.setScale(Vector3Dd(-2,3,1));
    EXPECT_EQ(GeometryContainment::Inside, body.classifyPoint(Vector3Dd(10,0,0), 0));
    EXPECT_EQ(GeometryContainment::Boundary, body.classifyPoint(Vector3Dd(8,0,0), 0));
    EXPECT_EQ(GeometryContainment::Boundary, body.classifyPoint(Vector3Dd(8.05,0,0), 0.1));
    body.setScale(Vector3Dd(-2,-2,-2));
    EXPECT_EQ(GeometryContainment::Boundary, body.classifyPoint(Vector3Dd(7.95,0,0), 0.1));
    EXPECT_EQ(GeometryContainment::InvalidInput, body.classifyPoint(Vector3Dd(), -1));
    body.setScale(Vector3Dd(0,2,2));
    EXPECT_EQ(GeometryContainment::InvalidInput, body.classifyPoint(Vector3Dd(), 0));
    body.setScale(Vector3Dd(1e-12,2,2));
    EXPECT_EQ(GeometryContainment::Indeterminate, body.classifyPoint(Vector3Dd(), 0));
    GeometryQueryWorkspace workspace;
    Ray ray = Ray::fromAffine(Vector3Dd(-3,0,0), Vector3Dd(1,0,0));
    EXPECT_EQ(RaySolidQueryStatus::Indeterminate, body.classifyRay(ray, RayCrossingQuery(0,5), workspace.solidResult()));
    body.setScale(Vector3Dd(java::Double::POSITIVE_INFINITY,2,2));
    EXPECT_EQ(GeometryContainment::InvalidInput, body.classifyPoint(Vector3Dd(), 0));
    EXPECT_EQ(RaySolidQueryStatus::InvalidInput, body.classifyRay(ray, RayCrossingQuery(0,5), workspace.solidResult()));
    body.setScale(Vector3Dd(1,1,1));
    body.setRotation(Matrix4x4d::identityMatrix().withVal(0,0,0));
    EXPECT_EQ(GeometryContainment::Indeterminate, body.classifyPoint(Vector3Dd(), 0));
    EXPECT_EQ(RaySolidQueryStatus::Indeterminate, body.classifyRay(ray, RayCrossingQuery(0,5), workspace.solidResult()));
    body.setRotation(Matrix4x4d::identityMatrix().withVal(0,1,0.5));
    EXPECT_EQ(GeometryContainment::Indeterminate, body.classifyPoint(Vector3Dd(), 0));
    EXPECT_EQ(RaySolidQueryStatus::Indeterminate, body.classifyRay(ray, RayCrossingQuery(0,5), workspace.solidResult()));
    body.setRotation(Matrix4x4d());
    body.setPosition(Vector3Dd(java::Double::POSITIVE_INFINITY,0,0));
    EXPECT_EQ(GeometryContainment::InvalidInput, body.classifyPoint(Vector3Dd(), 0));
    EXPECT_EQ(RaySolidQueryStatus::InvalidInput, body.classifyRay(ray, RayCrossingQuery(0,5), workspace.solidResult()));
    body.setPosition(Vector3Dd(1e308,0,0));
    body.setScale(Vector3Dd(1e-5,1,1));
    EXPECT_EQ(GeometryContainment::Indeterminate, body.classifyPoint(Vector3Dd(), 0));
    EXPECT_EQ(RaySolidQueryStatus::Indeterminate, body.classifyRay(ray, RayCrossingQuery(0,5), workspace.solidResult()));
}

TEST(GeometryQueryContract, EllipsoidUsesWorldSpaceToleranceOnMultipleAxes) {
    Sphere sphere(1);
    SimpleBody ellipsoid;
    ellipsoid.setGeometryReference(&sphere);
    ellipsoid.setPosition(Vector3Dd(10, 0, 0));
    ellipsoid.setScale(Vector3Dd(3, 2, 1));
    const double tolerance = 0.1;
    EXPECT_EQ(GeometryContainment::Inside, ellipsoid.classifyPoint(Vector3Dd(10, 0, 0), tolerance));
    EXPECT_EQ(GeometryContainment::Boundary, ellipsoid.classifyPoint(Vector3Dd(12.95, 0, 0), tolerance));
    EXPECT_EQ(GeometryContainment::Inside, ellipsoid.classifyPoint(Vector3Dd(12.8, 0, 0), tolerance));
    EXPECT_EQ(GeometryContainment::Boundary, ellipsoid.classifyPoint(Vector3Dd(13.05, 0, 0), tolerance));
    EXPECT_EQ(GeometryContainment::Outside, ellipsoid.classifyPoint(Vector3Dd(13.2, 0, 0), tolerance));
    EXPECT_EQ(GeometryContainment::Boundary, ellipsoid.classifyPoint(Vector3Dd(10, 0, 0.95), tolerance));
    EXPECT_EQ(GeometryContainment::Inside, ellipsoid.classifyPoint(Vector3Dd(10, 0, 0.8), tolerance));
    EXPECT_EQ(GeometryContainment::Boundary, ellipsoid.classifyPoint(Vector3Dd(10, 0, 1.05), tolerance));
    EXPECT_EQ(GeometryContainment::Outside, ellipsoid.classifyPoint(Vector3Dd(10, 0, 1.2), tolerance));
    EXPECT_EQ(GeometryContainment::Inside, ellipsoid.classifyPointCertified(Vector3Dd(10, 0, 0), tolerance));
    EXPECT_EQ(GeometryContainment::Boundary, ellipsoid.classifyPointCertified(Vector3Dd(12.95, 0, 0), tolerance));
    EXPECT_EQ(GeometryContainment::Inside, ellipsoid.classifyPointCertified(Vector3Dd(12.8, 0, 0), tolerance));
    EXPECT_EQ(GeometryContainment::Boundary, ellipsoid.classifyPointCertified(Vector3Dd(13.05, 0, 0), tolerance));
    EXPECT_EQ(GeometryContainment::Outside, ellipsoid.classifyPointCertified(Vector3Dd(13.2, 0, 0), tolerance));
    EXPECT_EQ(GeometryContainment::Boundary, ellipsoid.classifyPointCertified(Vector3Dd(10, 0, 0.95), tolerance));
    EXPECT_EQ(GeometryContainment::Inside, ellipsoid.classifyPointCertified(Vector3Dd(10, 0, 0.8), tolerance));
    EXPECT_EQ(GeometryContainment::Boundary, ellipsoid.classifyPointCertified(Vector3Dd(10, 0, 1.05), tolerance));
    EXPECT_EQ(GeometryContainment::Outside, ellipsoid.classifyPointCertified(Vector3Dd(10, 0, 1.2), tolerance));
    // Exactly on the tolerance threshold, floating point comparisons must not
    // silently force either boundary or outside when the certificate is unsure.
    EXPECT_EQ(GeometryContainment::Indeterminate, ellipsoid.classifyPoint(Vector3Dd(13.1, 0, 0), tolerance));
    ellipsoid.setScale(Vector3Dd(-3, 2, 1));
    EXPECT_EQ(GeometryContainment::Boundary, ellipsoid.classifyPoint(Vector3Dd(13.05, 0, 0), tolerance));
    EXPECT_EQ(GeometryContainment::Outside, ellipsoid.classifyPoint(Vector3Dd(13.2, 0, 0), tolerance));
    ellipsoid.setScale(Vector3Dd(0, 2, 1));
    EXPECT_EQ(GeometryContainment::InvalidInput, ellipsoid.classifyPoint(Vector3Dd(10, 0, 0), tolerance));
    ellipsoid.setScale(Vector3Dd(1e-12, 2, 1));
    EXPECT_EQ(GeometryContainment::Indeterminate, ellipsoid.classifyPoint(Vector3Dd(10, 0, 0), tolerance));
}

TEST(GeometryQueryContract, ShearedEllipsoidUsesWorldSpaceNormalDistance) {
    Sphere sphere(1);
    SimpleBody ellipsoid;
    ellipsoid.setGeometryReference(&sphere);
    const Matrix4x4d forward = Matrix4x4d().translation(3, 0, 0).multiply(
        Matrix4x4d::identityMatrix().withVal(0, 1, 0.5)).multiply(Matrix4x4d().scale(2, 1, 1));
    const Matrix4x4d inverse = Matrix4x4d().scale(0.5, 1, 1).multiply(
        Matrix4x4d::identityMatrix().withVal(0, 1, -0.5)).multiply(Matrix4x4d().translation(-3, 0, 0));
    ASSERT_EQ(AffinePlacementStatus::Valid, ellipsoid.setAffinePlacement(forward, inverse));
    // At the transformed local point (0, 1, 0), the world normal is (0, 1, 0).
    EXPECT_EQ(GeometryContainment::Boundary, ellipsoid.classifyPoint(Vector3Dd(3.5, 1.05, 0), 0.1));
    EXPECT_EQ(GeometryContainment::Outside, ellipsoid.classifyPoint(Vector3Dd(3.5, 1.2, 0), 0.1));
    EXPECT_EQ(GeometryContainment::Boundary, ellipsoid.classifyPoint(Vector3Dd(3.5, 0.95, 0), 0.1));
}

TEST(GeometryQueryContract, EllipsoidObliqueNormalUsesAWorldSpaceWitness) {
    Sphere sphere(1);
    SimpleBody ellipsoid;
    ellipsoid.setGeometryReference(&sphere);
    ellipsoid.setScale(Vector3Dd(3,2,1));
    // (1.8, 1.6, 0) lies on the surface; (1, 2, 0)/sqrt(5) is its
    // outward unit normal. Both offsets have an explicit surface witness.
    const double nx = 1.0 / java::Math::sqrt(5.0);
    const double ny = 2.0 * nx;
    EXPECT_EQ(GeometryContainment::Boundary,
        ellipsoid.classifyPoint(Vector3Dd(1.8 + 0.095*nx, 1.6 + 0.095*ny, 0), 0.1));
    EXPECT_EQ(GeometryContainment::Boundary,
        ellipsoid.classifyPoint(Vector3Dd(1.8 - 0.095*nx, 1.6 - 0.095*ny, 0), 0.1));
    EXPECT_EQ(GeometryContainment::Outside,
        ellipsoid.classifyPoint(Vector3Dd(1.8 + 0.101*nx, 1.6 + 0.101*ny, 0), 0.1));
}

TEST(GeometryQueryContract, EllipsoidInteriorNearToleranceIsNotBoundary) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    body.setScale(Vector3Dd(3,2,1));
    // For x > 3 - 1/3 the nearest surface point lies at (3,0,0),
    // so this interior point is 0.2 world units from the surface.
    EXPECT_EQ(GeometryContainment::Inside, body.classifyPointCertified(Vector3Dd(2.8,0,0), 0.19));
    EXPECT_EQ(GeometryContainment::Inside, body.classifyPoint(Vector3Dd(2.8,0,0), 0.19));
}

TEST(GeometryQueryContract, EllipsoidCenterCanReachBoundaryWithPositiveTolerance) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    body.setScale(Vector3Dd(3,2,1));
    EXPECT_EQ(GeometryContainment::Inside,body.classifyPoint(Vector3Dd(),0.9));
    EXPECT_EQ(GeometryContainment::Boundary,body.classifyPointCertified(Vector3Dd(),1.1));
    EXPECT_EQ(GeometryContainment::Boundary,body.classifyPoint(Vector3Dd(),1.1));
    EXPECT_EQ(GeometryContainment::Indeterminate,body.classifyPointCertified(Vector3Dd(),1.0));
    EXPECT_EQ(GeometryContainment::Indeterminate,body.classifyPoint(Vector3Dd(),1.0));
}

TEST(GeometryQueryContract, ExtremeAffineEllipsoidCertifiesBoundaryWhenPossible) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    const Matrix4x4d forward=Matrix4x4d().scale(1e-12,1,1);
    const Matrix4x4d inverse=Matrix4x4d().scale(1e12,1,1);
    ASSERT_EQ(AffinePlacementStatus::Valid,body.setAffinePlacement(forward,inverse));
    EXPECT_EQ(GeometryContainment::Inside,body.classifyPoint(Vector3Dd(),1e-15));
    EXPECT_EQ(GeometryContainment::Boundary,body.classifyPointCertified(Vector3Dd(1.05e-12,0,0),1e-13));
    EXPECT_EQ(GeometryContainment::Boundary,body.classifyPoint(Vector3Dd(1.05e-12,0,0),1e-13));
    EXPECT_EQ(GeometryContainment::Inside,body.classifyPoint(Vector3Dd(),0));
}

TEST(GeometryQueryContract, AffineEllipsoidAgreesWithIndependentSurfaceGrid) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    const Matrix4x4d forward = Matrix4x4d::identityMatrix().withVal(0,0,2).withVal(0,1,0.5);
    const Matrix4x4d inverse = Matrix4x4d::identityMatrix().withVal(0,0,0.5).withVal(0,1,-0.25);
    ASSERT_EQ(AffinePlacementStatus::Valid, body.setAffinePlacement(forward,inverse));
    const Vector3Dd points[] = {Vector3Dd(0,0,0), Vector3Dd(1.1,0.2,0.1),
        Vector3Dd(2.4,0.3,0.2), Vector3Dd(-1.8,0.9,0.4), Vector3Dd(0.5,-0.3,1.4)};
    int definite=0;
    for (int index=0;index<5;++index) {
        double sampled=java::Double::MAX_VALUE;
        for (int latitude=0;latitude<=180;++latitude) {
            const double theta=java::Math::PI*latitude/180;
            for (int longitude=0;longitude<360;++longitude) {
                const double phi=2*java::Math::PI*longitude/360;
                const Vector3Dd u(java::Math::sin(theta)*java::Math::cos(phi),
                    java::Math::sin(theta)*java::Math::sin(phi),java::Math::cos(theta));
                const double distance=forward.multiply(u).subtract(points[index]).length();
                if (distance<sampled) sampled=distance;
            }
        }
        // Every surface point is within 0.05 world units of a grid point:
        // ||A||_F < 2.6, and the sum of half angular steps is pi/180.
        const double errorBound=0.05;
        const GeometryContainment nearResult=body.classifyPoint(points[index],sampled+errorBound);
        EXPECT_TRUE(nearResult==GeometryContainment::Boundary || nearResult==GeometryContainment::Indeterminate);
        if (nearResult==GeometryContainment::Boundary) ++definite;
        if (sampled>2*errorBound) {
            const GeometryContainment side=body.classifyPoint(points[index],0);
            const GeometryContainment farResult=body.classifyPoint(points[index],sampled-2*errorBound);
            EXPECT_TRUE(farResult==side || farResult==GeometryContainment::Indeterminate);
            if (farResult==side) ++definite;
        }
    }
    EXPECT_GE(definite,5);
}

TEST(GeometryQueryContract, SurfaceGridChecksReflectionsShearAndScaleRanges) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    const Matrix4x4d rotation = Matrix4x4d::identityMatrix().withVal(0,0,0).withVal(0,1,-1)
        .withVal(1,0,1).withVal(1,1,0);
    const Matrix4x4d inverseRotation = Matrix4x4d::identityMatrix().withVal(0,0,0).withVal(0,1,1)
        .withVal(1,0,-1).withVal(1,1,0);
    const Matrix4x4d shear = Matrix4x4d::identityMatrix().withVal(0,1,0.5).withVal(1,2,-0.3);
    const Matrix4x4d inverseShear = Matrix4x4d::identityMatrix().withVal(0,1,-0.5)
        .withVal(1,2,0.3).withVal(0,2,-0.15);
    const Vector3Dd localPoints[] = {Vector3Dd(), Vector3Dd(0.6,0.2,0.1),
        Vector3Dd(1.2,0.4,0.3), Vector3Dd(-0.8,0.9,0.4), Vector3Dd(0.2,-0.4,1.4)};
    const double scales[] = {0.01,1,100};
    int definite=0;
    for (int mode=0; mode<3; ++mode) {
        const double factor=scales[mode];
        const Matrix4x4d forward=Matrix4x4d().translation(4,-3,2).multiply(rotation).multiply(shear)
            .multiply(Matrix4x4d().scale(-2*factor,factor,0.7*factor));
        const Matrix4x4d inverse=Matrix4x4d().scale(-0.5/factor,1/factor,1/(0.7*factor))
            .multiply(inverseShear).multiply(inverseRotation).multiply(Matrix4x4d().translation(-4,3,-2));
        ASSERT_EQ(AffinePlacementStatus::Valid,body.setAffinePlacement(forward,inverse));
        // ||A||_F < 3*factor; angular grid half-step sum is pi/180.
        const double errorBound=0.055*factor;
        for (int index=0; index<5; ++index) {
            const Vector3Dd point=forward.multiply(localPoints[index]);
            double sampled=java::Double::MAX_VALUE;
            for (int latitude=0; latitude<=180; ++latitude) {
                const double theta=java::Math::PI*latitude/180;
                for (int longitude=0; longitude<360; ++longitude) {
                    const double phi=2*java::Math::PI*longitude/360;
                    const Vector3Dd unit(java::Math::sin(theta)*java::Math::cos(phi),
                        java::Math::sin(theta)*java::Math::sin(phi),java::Math::cos(theta));
                    const double distance=forward.multiply(unit).subtract(point).length();
                    if (distance<sampled) sampled=distance;
                }
            }
            const GeometryContainment nearResult=body.classifyPoint(point,sampled+errorBound);
            EXPECT_TRUE(nearResult==GeometryContainment::Boundary || nearResult==GeometryContainment::Indeterminate);
            if (nearResult==GeometryContainment::Boundary) ++definite;
            if (sampled>2*errorBound) {
                const GeometryContainment side=localPoints[index].dotProduct(localPoints[index])<1
                    ? GeometryContainment::Inside : GeometryContainment::Outside;
                const GeometryContainment farResult=body.classifyPoint(point,sampled-2*errorBound);
                EXPECT_TRUE(farResult==side || farResult==GeometryContainment::Indeterminate);
                if (farResult==side) ++definite;
            }
        }
    }
    EXPECT_GE(definite,15);
}

TEST(GeometryQueryContract, ShearedEllipsoidObliqueNormalHasWorldDistance) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    const Matrix4x4d forward = Matrix4x4d::identityMatrix().withVal(0,0,2).withVal(0,1,0.5);
    const Matrix4x4d inverse = Matrix4x4d::identityMatrix().withVal(0,0,0.5).withVal(0,1,-0.25);
    ASSERT_EQ(AffinePlacementStatus::Valid, body.setAffinePlacement(forward, inverse));
    // The transformed point of (0.6, 0.8, 0) has normal proportional to (0.3, 0.65, 0).
    const double length = java::Math::sqrt(0.3*0.3 + 0.65*0.65);
    const double nx = 0.3 / length, ny = 0.65 / length;
    EXPECT_EQ(GeometryContainment::Boundary, body.classifyPoint(Vector3Dd(1.6+0.09*nx,0.8+0.09*ny,0),0.1));
    EXPECT_EQ(GeometryContainment::Boundary, body.classifyPoint(Vector3Dd(1.6-0.09*nx,0.8-0.09*ny,0),0.1));
    EXPECT_EQ(GeometryContainment::Outside, body.classifyPoint(Vector3Dd(1.6+0.11*nx,0.8+0.11*ny,0),0.1));
}

TEST(GeometryQueryContract, ExteriorNearBandUsesImprovedSupportingPlane) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    const Matrix4x4d forward=Matrix4x4d::identityMatrix().withVal(0,0,3).withVal(0,1,0.8);
    const Matrix4x4d inverse=Matrix4x4d::identityMatrix().withVal(0,0,1.0/3).withVal(0,1,-0.8/3);
    ASSERT_EQ(AffinePlacementStatus::Valid,body.setAffinePlacement(forward,inverse));
    // Transform the unit-sphere point (0.6, 0.8, 0). Convexity makes the
    // outward normal offset its exact distance to the transformed surface.
    const Vector3Dd surface=forward.multiply(Vector3Dd(0.6,0.8,0));
    const Vector3Dd normal(0.2,0.64,0);
    const Vector3Dd unit=normal.multiply(1.0/normal.length());
    EXPECT_EQ(GeometryContainment::Outside,body.classifyPointCertified(surface.add(unit.multiply(0.1001)),0.1));
    EXPECT_EQ(GeometryContainment::Outside,body.classifyPoint(surface.add(unit.multiply(0.1001)),0.1));
}

TEST(GeometryQueryContract, ShearedEllipsoidNormalOffsetsBracketTolerance) {
    Sphere sphere(1);
    SimpleBody body;
    body.setGeometryReference(&sphere);
    const Matrix4x4d forward=Matrix4x4d::identityMatrix().withVal(0,0,3).withVal(0,1,0.8);
    const Matrix4x4d inverse=Matrix4x4d::identityMatrix().withVal(0,0,1.0/3).withVal(0,1,-0.8/3);
    ASSERT_EQ(AffinePlacementStatus::Valid,body.setAffinePlacement(forward,inverse));
    const Vector3Dd source[] = {Vector3Dd(0.6,0.8,0),Vector3Dd(-0.8,0.6,0),
        Vector3Dd(0,0.6,0.8),Vector3Dd(0.6,0,0.8),Vector3Dd(-0.6,-0.8,0)};
    int definite=0;
    for (int i=0;i<5;++i) {
        const Vector3Dd p=forward.multiply(source[i]);
        const Vector3Dd n(source[i].x()/3,source[i].y()-0.8*source[i].x()/3,source[i].z());
        const Vector3Dd u=n.multiply(1.0/n.length());
        // Every outward normal ray of a convex ellipsoid has an exact
        // distance equal to its offset from the chosen surface point.
        const GeometryContainment outside=body.classifyPoint(p.add(u.multiply(0.1001)),0.1);
        EXPECT_TRUE(outside==GeometryContainment::Outside || outside==GeometryContainment::Indeterminate);
        if (outside==GeometryContainment::Outside) ++definite;
        const GeometryContainment near=body.classifyPoint(p.add(u.multiply(0.0999)),0.1);
        EXPECT_TRUE(near==GeometryContainment::Boundary || near==GeometryContainment::Indeterminate);
        if (near==GeometryContainment::Boundary) ++definite;
    }
    EXPECT_GE(definite,5);
}

TEST(GeometryQueryContract, RotatedReflectedEllipsoidUsesWorldTolerance) {
    Sphere sphere(1);
    SimpleBody ellipsoid;
    ellipsoid.setGeometryReference(&sphere);
    const Matrix4x4d rotation = Matrix4x4d::identityMatrix().withVal(0,0,0).withVal(0,1,-1)
        .withVal(1,0,1).withVal(1,1,0);
    const Matrix4x4d inverseRotation = Matrix4x4d::identityMatrix().withVal(0,0,0).withVal(0,1,1)
        .withVal(1,0,-1).withVal(1,1,0);
    const Matrix4x4d forward = rotation.multiply(Matrix4x4d().scale(-3,2,1));
    const Matrix4x4d inverse = Matrix4x4d().scale(-1.0/3,0.5,1).multiply(inverseRotation);
    ASSERT_EQ(AffinePlacementStatus::Valid, ellipsoid.setAffinePlacement(forward, inverse));
    EXPECT_EQ(GeometryContainment::Inside, ellipsoid.classifyPoint(Vector3Dd(), 0.1));
    EXPECT_EQ(GeometryContainment::Boundary, ellipsoid.classifyPoint(Vector3Dd(0,3.05,0), 0.1));
    EXPECT_EQ(GeometryContainment::Outside, ellipsoid.classifyPoint(Vector3Dd(0,3.2,0), 0.1));
}


TEST(GeometryQueryContract, OpenSurfaceDoesNotAcquireFictitiousSolidInterior) {
    InfinitePlane plane(1,0,0,0);
    const Ray ray = Ray::fromAffine(Vector3Dd(-2,0,0), Vector3Dd(1,0,0));
    RayHit legacy(RayHit::DETAIL_NONE, false);
    EXPECT_TRUE(plane.doIntersectionFirstHit(ray, &legacy));
    GeometryQueryWorkspace workspace;
    EXPECT_EQ(RayFirstHitStatus::Hit,
        plane.queryFirstHit(ray, RayCrossingQuery(0,4), &workspace.hitResult()));
    EXPECT_EQ(RaySolidQueryStatus::Unsupported,
        plane.classifyRay(ray, RayCrossingQuery(0,4), workspace.solidResult()));
    EXPECT_TRUE(workspace.solidResult().insideSegments.isEmpty());
}


TEST(GeometryQueryContract, SmallSphereNormalIsUnitAndZeroNormalUnavailable) {
    Sphere sphere(1e-4);
    Ray ray = Ray::fromAffine(Vector3Dd(0,0,3e-4), Vector3Dd(0,0,-1));
    RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL);
    ASSERT_EQ(RayFirstHitStatus::Hit, sphere.queryFirstHit(ray, RayCrossingQuery(0,1), &hit));
    EXPECT_NEAR(1, hit.normal.z(), 1e-12);
    EXPECT_EQ(static_cast<int>(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL), hit.getAvailableDetailMask());
    Sphere unit(1);
    Ray center = Ray::fromAffine(Vector3Dd(), Vector3Dd(1,0,0));
    hit.reset(RayHit::DETAIL_NORMAL);
    unit.doExtraInformation(center, 0, &hit);
    EXPECT_TRUE(hit.isDetailAvailabilityKnown());
    EXPECT_EQ(0, hit.getAvailableDetailMask() & RayHit::DETAIL_NORMAL);
}

TEST(GeometryBoundsContract, TransformsAllCornersAndReportsInvalidPlacement) {
    GeometryBounds local;
    ASSERT_TRUE(local.setFinite(Vector3Dd(1,2,3), Vector3Dd(3,4,5)));
    Matrix4x4d placement = Matrix4x4d::identityMatrix().withVal(0,0,-1)
        .withVal(0,1,2).withVal(0,3,10);
    const GeometryBounds world = local.transformed(placement);
    ASSERT_EQ(GeometryBounds::Kind::Finite, world.getKind());
    EXPECT_DOUBLE_EQ(11, world.getMinimum().x());
    EXPECT_DOUBLE_EQ(17, world.getMaximum().x());
    EXPECT_DOUBLE_EQ(2, world.getMinimum().y());
    EXPECT_DOUBLE_EQ(4, world.getMaximum().y());
    EXPECT_DOUBLE_EQ(3, world.getMinimum().z());
    EXPECT_DOUBLE_EQ(5, world.getMaximum().z());
    EXPECT_EQ(GeometryBounds::Kind::Indeterminate,
        local.transformed(placement.withVal(3,0,1)).getKind());
    EXPECT_EQ(GeometryBounds::Kind::Indeterminate,
        local.transformed(placement.withVal(0,0,java::Double::POSITIVE_INFINITY)).getKind());
    GeometryBounds empty;
    empty.setEmpty();
    EXPECT_EQ(GeometryBounds::Kind::Empty, empty.transformed(placement).getKind());
    GeometryBounds unbounded;
    unbounded.setUnbounded();
    EXPECT_EQ(GeometryBounds::Kind::Unbounded, unbounded.transformed(placement).getKind());
}

TEST(GeometryBoundsContract, BodyUsesItsPlacementAndGeometry) {
    SimpleBody body;
    body.setGeometry(new Sphere(2));
    body.setPosition(Vector3Dd(5,0,0));
    body.setScale(Vector3Dd(-2,1,1));
    GeometryBounds bounds;
    body.getWorldBounds(bounds);
    ASSERT_EQ(GeometryBounds::Kind::Finite, bounds.getKind());
    EXPECT_DOUBLE_EQ(1, bounds.getMinimum().x());
    EXPECT_DOUBLE_EQ(9, bounds.getMaximum().x());
    EXPECT_DOUBLE_EQ(-2, bounds.getMinimum().y());
    EXPECT_DOUBLE_EQ(2, bounds.getMaximum().y());
    SimpleBody empty;
    empty.getWorldBounds(bounds);
    EXPECT_EQ(GeometryBounds::Kind::Empty, bounds.getKind());
}

TEST(GeometryBoundsContract, EnclosingPreservesUnknownAndUnboundedStates) {
    GeometryBounds first, second, empty, unknown, indeterminate, unbounded;
    ASSERT_TRUE(first.setFinite(Vector3Dd(-2,1,3), Vector3Dd(1,4,5)));
    ASSERT_TRUE(second.setFinite(Vector3Dd(0,-3,4), Vector3Dd(5,2,7)));
    empty.setEmpty();
    indeterminate.setIndeterminate();
    unbounded.setUnbounded();
    const GeometryBounds combined = first.enclosing(second);
    ASSERT_EQ(GeometryBounds::Kind::Finite, combined.getKind());
    EXPECT_DOUBLE_EQ(-2, combined.getMinimum().x());
    EXPECT_DOUBLE_EQ(-3, combined.getMinimum().y());
    EXPECT_DOUBLE_EQ(7, combined.getMaximum().z());
    EXPECT_EQ(GeometryBounds::Kind::Finite, empty.enclosing(first).getKind());
    EXPECT_EQ(GeometryBounds::Kind::Unbounded, first.enclosing(unbounded).getKind());
    EXPECT_EQ(GeometryBounds::Kind::Unsupported, unbounded.enclosing(unknown).getKind());
    EXPECT_EQ(GeometryBounds::Kind::Indeterminate, unknown.enclosing(indeterminate).getKind());
}

#include "vsdk/toolkit/environment/geometry/volume/QuadricSurface.h"
TEST(QuadricSurfaceContract, EllipsoidAndParaboloidUseAffineRoots) {
    QuadricSurface ellipsoid(Vector3Dd(1,4,9), Vector3Dd(), Vector3Dd(), -1);
    const Ray ray = Ray::fromAffine(Vector3Dd(-3,0,0), Vector3Dd(2,0,0));
    RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL, false);
    RayBoundary boundary;
    ASSERT_EQ(RayFirstHitStatus::Hit, ellipsoid.queryFirstHit(ray, RayCrossingQuery(0,10), &hit, &boundary));
    EXPECT_DOUBLE_EQ(1, boundary.t);
    EXPECT_EQ(&ellipsoid, boundary.geometry);
    EXPECT_DOUBLE_EQ(-1, hit.point.x());
    EXPECT_DOUBLE_EQ(-1, hit.normal.x());
    RayHit deferred(RayHit::DETAIL_NORMAL, false);
    boundary.normalReversed = true;
    ASSERT_EQ(RayFirstHitStatus::Hit, ellipsoid.querySurfaceDetail(ray, boundary, &deferred));
    EXPECT_DOUBLE_EQ(1, deferred.normal.x());
    RayCrossingTestCollector events;
    ASSERT_EQ(RayCrossingSupport::Supported, ellipsoid.emitRayCrossings(ray, RayCrossingQuery(0,10), events));
    ASSERT_EQ(2, events.roots.size());
    EXPECT_DOUBLE_EQ(1, events.roots[0]);
    EXPECT_DOUBLE_EQ(2, events.roots[1]);
    EXPECT_EQ(GeometryContainment::Inside, ellipsoid.classifyPoint(Vector3Dd(),0));
    QuadricSurface paraboloid(Vector3Dd(1,1,0), Vector3Dd(), Vector3Dd(0,0,-1), 0);
    const Ray vertical=Ray::fromAffine(Vector3Dd(1,0,3), Vector3Dd(0,0,-2));
    ASSERT_EQ(RayFirstHitStatus::Hit, paraboloid.queryFirstHit(vertical,RayCrossingQuery(0,10),&hit,&boundary));
    EXPECT_DOUBLE_EQ(1, boundary.t);
    RayCrossingTestCollector verticalEvents;
    ASSERT_EQ(RayCrossingSupport::Supported, paraboloid.emitRayCrossings(vertical, RayCrossingQuery(0,10), verticalEvents));
    ASSERT_EQ(1, verticalEvents.roots.size());
    EXPECT_EQ(RayCrossingKind::Leaving, verticalEvents.kinds[0]);
    QuadricSurface plane(Vector3Dd(), Vector3Dd(), Vector3Dd(0,0,1), 0);
    EXPECT_EQ(RayFirstHitStatus::Indeterminate, plane.queryFirstHit(
        Ray::fromAffine(Vector3Dd(),Vector3Dd(1,0,0)), RayCrossingQuery(0,10), &hit, &boundary));
}

TEST(QuadricSurfaceContract, SolidIntervalsHandleEllipsoidsComplementsAndClipping) {
    QuadricSurface solid(Vector3Dd(1,1,1), Vector3Dd(), Vector3Dd(), -1);
    QuadricSurface complement(Vector3Dd(-1,-1,-1), Vector3Dd(), Vector3Dd(), 1);
    const Ray ray = Ray::fromAffine(Vector3Dd(-3,0,0),Vector3Dd(1,0,0));
    RaySolidClassification result;
    ASSERT_EQ(RaySolidQueryStatus::Supported, solid.classifyRay(ray,RayCrossingQuery(0,10),result));
    ASSERT_EQ(2, result.boundaries.size());
    ASSERT_EQ(3, result.insideSegments.size());
    EXPECT_DOUBLE_EQ(2, result.boundaries[0].t);
    EXPECT_DOUBLE_EQ(4, result.boundaries[1].t);
    EXPECT_EQ(&solid, result.boundaries[0].geometry);
    EXPECT_FALSE(result.insideSegments[0]); EXPECT_TRUE(result.insideSegments[1]); EXPECT_FALSE(result.insideSegments[2]);
    ASSERT_EQ(RaySolidQueryStatus::Supported, complement.classifyRay(ray,RayCrossingQuery(0,10),result));
    ASSERT_EQ(3, result.insideSegments.size());
    EXPECT_TRUE(result.insideSegments[0]); EXPECT_FALSE(result.insideSegments[1]); EXPECT_TRUE(result.insideSegments[2]);
    ASSERT_EQ(RaySolidQueryStatus::Supported, solid.classifyRay(ray,RayCrossingQuery(2.5,3.5),result));
    EXPECT_EQ(0, result.boundaries.size()); ASSERT_EQ(1, result.insideSegments.size()); EXPECT_TRUE(result.insideSegments[0]);
    ASSERT_EQ(RaySolidQueryStatus::Supported, solid.classifyRay(ray,RayCrossingQuery(2,4),result));
    ASSERT_EQ(3, result.insideSegments.size());
    EXPECT_FALSE(result.insideSegments[0]); EXPECT_TRUE(result.insideSegments[1]); EXPECT_FALSE(result.insideSegments[2]);
}

TEST(QuadricSurfaceContract, SolidIntervalsHandleLinearTangencyAndCoincidence) {
    QuadricSurface paraboloid(Vector3Dd(1,1,0), Vector3Dd(), Vector3Dd(0,0,-1), 0);
    RaySolidClassification result;
    const Ray vertical=Ray::fromAffine(Vector3Dd(1,0,3),Vector3Dd(0,0,-2));
    ASSERT_EQ(RaySolidQueryStatus::Supported, paraboloid.classifyRay(vertical,RayCrossingQuery(0,10),result));
    ASSERT_EQ(1, result.boundaries.size()); ASSERT_EQ(2, result.insideSegments.size());
    EXPECT_DOUBLE_EQ(1, result.boundaries[0].t);
    EXPECT_TRUE(result.insideSegments[0]); EXPECT_FALSE(result.insideSegments[1]);
    QuadricSurface sphere(Vector3Dd(1,1,1),Vector3Dd(),Vector3Dd(),-1);
    QuadricSurface inverse(Vector3Dd(-1,-1,-1),Vector3Dd(),Vector3Dd(),1);
    const Ray tangent=Ray::fromAffine(Vector3Dd(-3,1,0),Vector3Dd(1,0,0));
    ASSERT_EQ(RaySolidQueryStatus::Supported, sphere.classifyRay(tangent,RayCrossingQuery(0,10),result));
    EXPECT_EQ(0, result.boundaries.size()); ASSERT_EQ(1, result.insideSegments.size()); EXPECT_FALSE(result.insideSegments[0]);
    ASSERT_EQ(RaySolidQueryStatus::Supported, inverse.classifyRay(tangent,RayCrossingQuery(0,10),result));
    ASSERT_EQ(1, result.insideSegments.size()); EXPECT_TRUE(result.insideSegments[0]);
    QuadricSurface plane(Vector3Dd(),Vector3Dd(),Vector3Dd(0,0,1),0);
    EXPECT_EQ(RaySolidQueryStatus::Indeterminate, plane.classifyRay(
        Ray::fromAffine(Vector3Dd(),Vector3Dd(1,0,0)),RayCrossingQuery(0,10),result));
    EXPECT_EQ(0, result.insideSegments.size()); EXPECT_EQ(0, result.boundaries.size());
}

TEST(QuadricSurfaceContract, SolidIntervalsPreserveAffineInstanceProvenance) {
    SimpleBody body;
    QuadricSurface* quadric = new QuadricSurface(Vector3Dd(1,1,1),Vector3Dd(),Vector3Dd(),-1);
    body.setGeometry(quadric);
    body.setPosition(Vector3Dd(3,0,0));
    body.setScale(Vector3Dd(2,1,1));
    const Ray ray = Ray::fromAffine(Vector3Dd(-1,0,0),Vector3Dd(2,0,0));
    RaySolidClassification result;
    ASSERT_EQ(RaySolidQueryStatus::Supported, body.classifyRay(ray,RayCrossingQuery(0,10),result));
    ASSERT_EQ(2, result.boundaries.size());
    EXPECT_DOUBLE_EQ(1, result.boundaries[0].t);
    EXPECT_DOUBLE_EQ(3, result.boundaries[1].t);
    EXPECT_EQ(quadric, result.boundaries[0].geometry);
    EXPECT_EQ(1, result.boundaries[0].getInstanceCount());
}

TEST(QuadricSurfaceContract, FiniteBoundsEncloseShiftedMixedEllipsoid) {
    QuadricSurface quadric(Vector3Dd(2,2,4),Vector3Dd(2,0,0),Vector3Dd(0,6,-4),-2);
    GeometryBounds bounds;
    quadric.getBounds(bounds);
    ASSERT_EQ(GeometryBounds::Kind::Finite, bounds.getKind());
    const double extent=java::Math::sqrt(6.0);
    EXPECT_NEAR(1-extent,bounds.getMinimum().x(),1e-12);
    EXPECT_NEAR(1+extent,bounds.getMaximum().x(),1e-12);
    EXPECT_NEAR(-2-extent,bounds.getMinimum().y(),1e-12);
    EXPECT_NEAR(-2+extent,bounds.getMaximum().y(),1e-12);
    EXPECT_NEAR(-1,bounds.getMinimum().z(),1e-12);
    EXPECT_NEAR(2,bounds.getMaximum().z(),1e-12);
    double* legacy=quadric.getMinMax();
    EXPECT_DOUBLE_EQ(bounds.getMinimum().x(),legacy[0]);
    EXPECT_DOUBLE_EQ(bounds.getMaximum().z(),legacy[5]);
    delete[] legacy;
    SimpleBody body;
    body.setGeometryReference(&quadric);
    body.setPosition(Vector3Dd(4,0,0));
    body.setScale(Vector3Dd(-2,1,1));
    body.getWorldBounds(bounds);
    ASSERT_EQ(GeometryBounds::Kind::Finite,bounds.getKind());
    EXPECT_NEAR(2-2*extent,bounds.getMinimum().x(),1e-11);
    EXPECT_NEAR(2+2*extent,bounds.getMaximum().x(),1e-11);
}

TEST(QuadricSurfaceContract, BoundsAreConservativeForUnboundedAndInvalidInputs) {
    GeometryBounds bounds;
    QuadricSurface inverse(Vector3Dd(-1,-1,-1),Vector3Dd(),Vector3Dd(),1);
    inverse.getBounds(bounds); EXPECT_EQ(GeometryBounds::Kind::Unbounded,bounds.getKind());
    QuadricSurface cylinder(Vector3Dd(1,1,0),Vector3Dd(),Vector3Dd(),-1);
    cylinder.getBounds(bounds); EXPECT_EQ(GeometryBounds::Kind::Unbounded,bounds.getKind());
    QuadricSurface tiny(Vector3Dd(1e-15,1,1),Vector3Dd(),Vector3Dd(),-1);
    tiny.getBounds(bounds); EXPECT_EQ(GeometryBounds::Kind::Unbounded,bounds.getKind());
    QuadricSurface invalid(Vector3Dd(java::Double::POSITIVE_INFINITY,1,1),Vector3Dd(),Vector3Dd(),-1);
    invalid.getBounds(bounds); EXPECT_EQ(GeometryBounds::Kind::Indeterminate,bounds.getKind());
    QuadricSurface overflow(Vector3Dd(1e200,1e200,1e200),Vector3Dd(),Vector3Dd(),-1);
    overflow.getBounds(bounds); EXPECT_EQ(GeometryBounds::Kind::Indeterminate,bounds.getKind());
}

TEST(QuadricSurfaceContract, MixedTermSolidIntervalsAgreeWithAnalyticRoots) {
    QuadricSurface quadric(Vector3Dd(2,2,4),Vector3Dd(2,0,0),Vector3Dd(0,6,-4),-2);
    const Ray ray=Ray::fromAffine(Vector3Dd(-3,0,0),Vector3Dd(1,0,0));
    RaySolidClassification result;
    ASSERT_EQ(RaySolidQueryStatus::Supported,quadric.classifyRay(ray,RayCrossingQuery(0,10),result));
    ASSERT_EQ(2,result.boundaries.size()); ASSERT_EQ(3,result.insideSegments.size());
    EXPECT_DOUBLE_EQ(2,result.boundaries[0].t); EXPECT_DOUBLE_EQ(4,result.boundaries[1].t);
    EXPECT_FALSE(result.insideSegments[0]); EXPECT_TRUE(result.insideSegments[1]); EXPECT_FALSE(result.insideSegments[2]);
}

TEST(BoxContract, ArbitraryBoundsSupportSharedSolidQueriesAndDeferredDetail) {
    Box box(Vector3Dd(-2,-1,-3),Vector3Dd(4,3,1));
    const Ray ray=Ray::fromAffine(Vector3Dd(-4,0,0),Vector3Dd(1,0,0));
    RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL, false);
    RayBoundary boundary;
    ASSERT_EQ(RayFirstHitStatus::Hit,box.queryFirstHit(ray,RayCrossingQuery(0,10),&hit,&boundary));
    EXPECT_DOUBLE_EQ(2,boundary.t); EXPECT_EQ(&box,boundary.geometry);
    EXPECT_DOUBLE_EQ(-2,hit.point.x()); EXPECT_DOUBLE_EQ(-1,hit.normal.x());
    boundary.normalReversed=true;
    RayHit deferred(RayHit::DETAIL_NORMAL,false);
    ASSERT_EQ(RayFirstHitStatus::Hit,box.querySurfaceDetail(ray,boundary,&deferred));
    EXPECT_DOUBLE_EQ(1,deferred.normal.x());
    RaySolidClassification result;
    ASSERT_EQ(RaySolidQueryStatus::Supported,box.classifyRay(ray,RayCrossingQuery(0,10),result));
    ASSERT_EQ(2,result.boundaries.size()); ASSERT_EQ(3,result.insideSegments.size());
    EXPECT_DOUBLE_EQ(2,result.boundaries[0].t); EXPECT_DOUBLE_EQ(8,result.boundaries[1].t);
    EXPECT_FALSE(result.insideSegments[0]); EXPECT_TRUE(result.insideSegments[1]); EXPECT_FALSE(result.insideSegments[2]);
    EXPECT_EQ(GeometryContainment::Inside,box.classifyPoint(Vector3Dd(0,0,0),0));
    ASSERT_TRUE(box.doIntersectionFirstHit(ray,&hit)); EXPECT_DOUBLE_EQ(-2,hit.point.x());
    Geometry* clone=box.copyGeometry(); ASSERT_NE(nullptr,clone);
    GeometryBounds copied; clone->getBounds(copied);
    ASSERT_EQ(GeometryBounds::Kind::Finite,copied.getKind());
    EXPECT_DOUBLE_EQ(-2,copied.getMinimum().x()); EXPECT_DOUBLE_EQ(4,copied.getMaximum().x());
    delete clone;
    box.getRawBounds()[0]=Vector3Dd(-1,-1,-3);
    ASSERT_EQ(RayFirstHitStatus::Hit,box.queryFirstHit(ray,RayCrossingQuery(0,10),nullptr,&boundary));
    EXPECT_DOUBLE_EQ(3,boundary.t);
}

TEST(PlanarHalfSpaceContract, DeferredDetailPreservesLeafAndOrientation) {
    PlanarHalfSpace solid(2,0,0,-4);
    const Ray ray=Ray::fromAffine(Vector3Dd(-3,0,0),Vector3Dd(1,0,0));
    RayBoundary boundary;
    ASSERT_EQ(RayFirstHitStatus::Hit,solid.queryFirstHit(ray,RayCrossingQuery(0,10),nullptr,&boundary));
    EXPECT_EQ(&solid,boundary.geometry); EXPECT_DOUBLE_EQ(5,boundary.t);
    boundary.normalReversed=true;
    RayHit detail(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL,false);
    ASSERT_EQ(RayFirstHitStatus::Hit,solid.querySurfaceDetail(ray,boundary,&detail));
    EXPECT_DOUBLE_EQ(2,detail.point.x()); EXPECT_DOUBLE_EQ(-1,detail.normal.x());
}

#include "vsdk/toolkit/environment/scene/csg/CsgBodyOperand.h"
#include "vsdk/toolkit/environment/scene/csg/CsgTree.h"
TEST(CsgStructureContract, OrderedBorrowedOperandsAndNestedTreesPreserveIdentity) {
    SimpleBody first, second;
    first.setGeometry(new Sphere(1));
    second.setGeometry(new Sphere(2));
    second.setPosition(Vector3Dd(4,0,0));
    CsgBodyOperand firstOperand(first), secondOperand(second);
    CsgTree difference(CsgOperation::DIFFERENCE);
    ASSERT_TRUE(difference.addOperand(firstOperand)); ASSERT_TRUE(difference.addOperand(secondOperand));
    EXPECT_EQ(CsgOperation::DIFFERENCE,difference.getOperation());
    ASSERT_EQ(2,difference.getOperandCount());
    EXPECT_EQ(first.getGeometry(),difference.getOperand(0)->getGeometry());
    EXPECT_EQ(second.getGeometry(),difference.getOperand(1)->getGeometry());
    EXPECT_EQ(nullptr,difference.getOperand(0)->getChildExpression());
    EXPECT_DOUBLE_EQ(4,difference.getOperand(1)->getAffinePlacement().multiply(Vector3Dd()).x());
    Matrix4x4d forward, inverse;
    ASSERT_EQ(AffinePlacementStatus::Valid,difference.getOperand(1)->queryAffinePlacement(forward,inverse));
    EXPECT_DOUBLE_EQ(-4,inverse.multiply(Vector3Dd()).x());
    second.setPosition(Vector3Dd(7,0,0));
    EXPECT_DOUBLE_EQ(7,difference.getOperand(1)->getAffinePlacement().multiply(Vector3Dd()).x());
    EXPECT_EQ(nullptr,difference.getOperand(-1)); EXPECT_EQ(nullptr,difference.getOperand(2));
    EXPECT_FALSE(difference.addOperand(difference));
    CsgTree root(CsgOperation::UNION);
    ASSERT_TRUE(root.addOperand(difference));
    EXPECT_EQ(&difference,root.getOperand(0)->getChildExpression());
    EXPECT_EQ(nullptr,root.getOperand(0)->getGeometry());
    EXPECT_EQ(CsgOperation::DIFFERENCE,root.getOperand(0)->getChildExpression()->getOperation());
    EXPECT_FALSE(difference.addOperand(root));
    EXPECT_EQ(2,difference.getOperandCount());
}

#include "vsdk/toolkit/environment/scene/AffinePlacement.h"
TEST(AffinePlacementContract, ValidatesPairsAndRejectsInvalidOrInconsistentMatrices) {
    const Matrix4x4d forward=Matrix4x4d().translation(3,-2,1).multiply(
        Matrix4x4d::identityMatrix().withVal(0,1,0.5));
    const Matrix4x4d inverse=Matrix4x4d::identityMatrix().withVal(0,1,-0.5).multiply(
        Matrix4x4d().translation(-3,2,-1));
    EXPECT_EQ(AffinePlacementStatus::Valid,AffinePlacement::validate(forward,inverse));
    EXPECT_EQ(AffinePlacementStatus::Invalid,AffinePlacement::validate(
        Matrix4x4d().scale(0,1,1),Matrix4x4d()));
    EXPECT_EQ(AffinePlacementStatus::Indeterminate,AffinePlacement::validate(forward,Matrix4x4d()));
    EXPECT_EQ(AffinePlacementStatus::Indeterminate,AffinePlacement::validate(
        Matrix4x4d().scale(1e-200,1e-200,1),Matrix4x4d().scale(1e200,1e200,1)));
    EXPECT_EQ(AffinePlacementStatus::Invalid,AffinePlacement::validate(
        forward.withVal(3,0,1),inverse));
    EXPECT_EQ(AffinePlacementStatus::Invalid,AffinePlacement::validate(
        forward.withVal(0,0,java::Double::POSITIVE_INFINITY),inverse));
}

TEST(AffinePlacementContract, SceneCrossingsKeepParametersAndInstanceProvenance) {
    SimpleBody body;
    Sphere* sphere=new Sphere(1);
    body.setGeometry(sphere);
    body.setPosition(Vector3Dd(3,0,0));
    body.setScale(Vector3Dd(-2,1,1));
    const Ray ray=Ray::fromAffine(Vector3Dd(-1,0,0),Vector3Dd(2,0,0));
    RayCrossingTestCollector result;
    ASSERT_EQ(RayCrossingSupport::Supported,body.emitRayCrossings(ray,RayCrossingQuery(0,10),result));
    ASSERT_EQ(2,result.roots.size());
    EXPECT_DOUBLE_EQ(1,result.roots[0]); EXPECT_DOUBLE_EQ(3,result.roots[1]);
    EXPECT_EQ(sphere,result.geometries[0]); EXPECT_EQ(1,result.instanceCounts[0]);
    EXPECT_EQ(RayCrossingKind::Entering,result.kinds[0]); EXPECT_EQ(RayCrossingKind::Leaving,result.kinds[1]);
}

TEST(AffinePlacementContract, ExplicitShearDrivesQueriesRenderingAndEditing) {
    SimpleBody body;
    Sphere* sphere=new Sphere(1);
    body.setGeometry(sphere);
    const Matrix4x4d forward=Matrix4x4d().translation(3,0,0).multiply(
        Matrix4x4d::identityMatrix().withVal(0,1,0.5)).multiply(Matrix4x4d().scale(2,1,1));
    const Matrix4x4d inverse=Matrix4x4d().scale(0.5,1,1).multiply(
        Matrix4x4d::identityMatrix().withVal(0,1,-0.5)).multiply(Matrix4x4d().translation(-3,0,0));
    ASSERT_EQ(AffinePlacementStatus::Valid,body.setAffinePlacement(forward,inverse));
    EXPECT_FALSE(body.supportsDecomposedPlacement());
    const Ray ray=Ray::fromAffine(forward.multiply(Vector3Dd(-3,0,0)),
        forward.withoutTranslation().multiply(Vector3Dd(1,0,0)));
    RaySolidClassification classification;
    ASSERT_EQ(RaySolidQueryStatus::Supported,body.classifyRay(ray,RayCrossingQuery(0,10),classification));
    ASSERT_EQ(2,classification.boundaries.size());
    EXPECT_NEAR(2,classification.boundaries[0].t,1e-12);
    RayHit hit(RayHit::DETAIL_POINT | RayHit::DETAIL_NORMAL, true);
    ASSERT_TRUE(body.doIntersectionFirstHit(ray,&hit));
    ASSERT_NE(nullptr,hit.getRay()); EXPECT_NEAR(2,hit.getRay()->getT(),1e-12);
    EXPECT_NEAR(1,hit.point.x(),1e-12);
    EXPECT_NEAR(-0.8944271909999159,hit.normal.x(),1e-12);
    EXPECT_NEAR(0.4472135954999579,hit.normal.y(),1e-12);
    EXPECT_EQ(GeometryContainment::Inside,body.classifyPoint(forward.multiply(Vector3Dd()),0));
    EXPECT_EQ(GeometryContainment::Inside,body.classifyPoint(forward.multiply(Vector3Dd()),0.1));
    const Matrix4x4d before=body.getAffinePlacement();
    EXPECT_EQ(AffinePlacementStatus::Invalid,body.setAffinePlacement(Matrix4x4d().scale(0,1,1),Matrix4x4d()));
    EXPECT_TRUE(body.getAffinePlacement().epsilonEquals(before,0));
    body.setPosition(Vector3Dd(5,1,0));
    Matrix4x4d moved, movedInverse;
    ASSERT_EQ(AffinePlacementStatus::Valid,body.queryAffinePlacement(moved,movedInverse));
    EXPECT_NEAR(5,moved.multiply(Vector3Dd()).x(),1e-12);
    EXPECT_NEAR(0,movedInverse.multiply(Vector3Dd(5,1,0)).length(),1e-12);
    body.resetAffinePlacement(); EXPECT_TRUE(body.supportsDecomposedPlacement());
    EXPECT_TRUE(body.getAffinePlacement().epsilonEquals(Matrix4x4d(),0));
}

TEST(FloatingIntervalContract, DeterministicStressContainsExactRationalBounds) {
    ASSERT_TRUE(FloatingInterval::isSupportedEnvironment());
    // Generated by generate_interval_stress_reference.py using exact rationals.
    const double cases[][5]={
        {7.146546754040844e-69, 7.146546754040845e-69, 0, 1.4293093508081688e-68, 1.429309350808169e-68},
        {-3.5566057525170395e+21, -1.4583237856775876e+33, 1, 1.4583237856740308e+33, 1.458323785674031e+33},
        {-8.11391527557009e+84, -1.5651942759869695e-82, 2, 1269.985374516554, 1269.9853745165542},
        {-7.005861629145818e+116, 2.093032075091621e+26, 3, -3.347230896516071e+90, -3.3472308965160706e+90},
        {1.7111650208095105e-80, 3951736602112120.0, 0, 3951736602112120.0, 3951736602112120.5},
        {-3.899596746493458e-148, -1.4313098573698764e+53, 1, 1.4313098573698762e+53, 1.4313098573698764e+53},
        {-2.9147068561821865e+67, 3.2035585822490825e-138, 2, -9.337434163862687e-71, -9.337434163862685e-71},
        {-1.0036314697615076e+56, 8.595075937226869e-51, 3, -1.1676819112377978e+106, -1.1676819112377976e+106},
        {-1.690969726056025e-14, -5.55933477059183e+44, 0, -5.559334770591831e+44, -5.55933477059183e+44},
        {-4.3388294890186107e+42, -1.7015852866887574e+116, 1, 1.701585286688757e+116, 1.7015852866887574e+116},
        {-1.894326392189219e+53, 2.329431193695396e-17, 2, -4.412702989006026e+36, -4.412702989006025e+36},
        {-1.0208457775245834e+153, -1.0208457775245832e+153, 3, 1.0, 1.0000000000000002},
        {-4.562818881199697e+85, 2.7172586259056785e-72, 0, -4.562818881199697e+85, -4.562818881199696e+85},
        {2.960898220504463e-40, 6.649198969387046e-112, 1, 2.9608982205044627e-40, 2.960898220504463e-40},
        {-6646668405780803.0, 2.848094402563231e+110, 2, -1.893033908219818e+126, -1.8930339082198176e+126},
        {8.454772843731548e-50, 1.1089667819206423e+73, 3, 7.624009106105553e-123, 7.624009106105555e-123},
        {2.8086253420183986e+58, -1.0792115406317105e-124, 0, 2.808625342018398e+58, 2.8086253420183986e+58},
        {1.5882786408807579e-99, 2.964314447997346e-102, 1, 1.5853143264327605e-99, 1.5853143264327607e-99},
        {-2.680570190843948e+104, 6.248135193642002e+67, 2, -1.6748564948439732e+172, -1.674856494843973e+172},
        {-4.931861314370597e-21, -3.391881745352832e-46, 3, 1.4540192390632924e+25, 1.4540192390632926e+25},
        {1.3741384403734075e+70, -2.044099822357928e+149, 0, -2.044099822357928e+149, -2.0440998223579277e+149},
        {7.652799199332246e+21, 3.9278002242702226e-89, 1, 7.652799199332245e+21, 7.652799199332246e+21},
        {3.75199107563365e-22, 3.7519910756336506e-22, 2, 1.4077437031634557e-43, 1.4077437031634559e-43},
        {-9.094398980472972e-147, 1.3287552253160717e-133, 3, -6.844299692826934e-14, -6.844299692826933e-14},
        {1.0265886784318475e+108, -1.916315795976818e+61, 0, 1.0265886784318473e+108, 1.0265886784318475e+108},
        {1.1188068606861152e+53, 1.2393022779389087e+70, 1, -1.2393022779389087e+70, -1.2393022779389085e+70},
        {4.027621290101606e-63, 7.831154528608348e-129, 2, 3.154092470549859e-191, 3.1540924705498595e-191},
        {1.4998938791684562e+66, -1.0134956380621436e-78, 3, -1.4799213956523103e+144, -1.4799213956523101e+144},
        {-3.291038874024351e+20, -4.5813304442681997e-125, 0, -3.2910388740243515e+20, -3.291038874024351e+20},
        {6.628538733888976e-24, -154744931.69699302, 1, 154744931.69699302, 154744931.69699305},
        {-9.205481630175304e-152, -1.3587824594568078e+55, 2, 1.2508246969934063e-96, 1.2508246969934066e-96},
        {-7.960151267585567e-87, -7.998774084540255e-37, 3, 9.951714079499587e-51, 9.951714079499588e-51},
        {-4.630168634212488e+158, -4.834134258630453e-18, 0, -4.630168634212489e+158, -4.630168634212488e+158},
        {3.847223730397442e-117, 3.847223730397443e-117, 1, -7.044203657368268e-133, -7.044203657368268e-133},
        {-6.833021388605202e-77, -2.7067301603686296e+107, 2, 1.8495145078981634e+31, 1.8495145078981637e+31},
        {-2.715791966617085e-97, -1.816861271330557e-46, 3, 1.4947712351357492e-51, 1.4947712351357495e-51},
        {7.971032764418196e+55, -7.402207684804431e-112, 0, 7.971032764418195e+55, 7.971032764418196e+55},
        {-4.882773892133515e+24, 4.0718275219942044e-82, 1, -4.882773892133516e+24, -4.882773892133515e+24},
        {-2.4793688191233346e-64, 2.3867158809198215e-110, 2, -5.917548935259088e-174, -5.917548935259087e-174},
        {4.947951554127676e+144, 3.666361594251627e+40, 3, 1.3495536179206691e+104, 1.3495536179206693e+104},
        {-2.3530147634202964e-72, -5.951630108185943e-73, 0, -2.948177774238891e-72, -2.9481777742388903e-72},
        {1.5835292443330722e-25, 5.230194477104014e-79, 1, 1.583529244333072e-25, 1.5835292443330722e-25},
        {-8.439740587736398e-115, 9.917447007016836e-09, 2, -8.370068003184486e-123, -8.370068003184485e-123},
        {2.1679518598992123e-100, 8.955389978715759e-95, 3, 2.4208346761579058e-06, 2.420834676157906e-06},
        {-3.248475655729729e-55, -3.2484756557297285e-55, 0, -6.496951311459458e-55, -6.496951311459457e-55},
        {-2.5154338451103455e-142, 1.2809559014651951e-48, 1, -1.2809559014651952e-48, -1.2809559014651951e-48},
        {-370705540480549.56, -1.6058651255974986e+60, 2, 5.953030993234863e+74, 5.953030993234864e+74},
        {-8.912909900324393e-70, -2.392715657549679e+78, 3, 3.725018420889962e-148, 3.725018420889963e-148},
        {7.521179311428108e+19, 4.300536432303738e-10, 0, 7.521179311428108e+19, 7.52117931142811e+19},
        {-4.6044121992001574e-126, -1.321209905758332e+67, 1, 1.3212099057583318e+67, 1.321209905758332e+67},
        {-5.8223902096831565e+134, 1.466369558975731e-114, 2, -8.537775763957706e+20, -8.537775763957704e+20},
        {5.0903072198465135e+120, -1.4937149639888107e+84, 3, -3.407816981529982e+36, -3.407816981529981e+36},
        {8.526842899455951e-27, -1.999149353405858e-07, 0, -1.999149353405858e-07, -1.9991493534058578e-07},
        {-4.092137893802181e+140, 1.5403752585329928e+89, 1, -4.092137893802182e+140, -4.092137893802181e+140},
        {-1.156487437800255e-147, -1.5947101103409532e-31, 2, 1.8442622095423708e-178, 1.844262209542371e-178},
        {1.696053312351897e+104, 1.6960533123518972e+104, 3, 0.9999999999999998, 0.9999999999999999},
        {-2.5248682808900746e-08, -4.261666906767745e+16, 0, -4.261666906767746e+16, -4.261666906767745e+16},
        {5.2751611350313e+116, 0.001492527481737261, 1, 5.2751611350312994e+116, 5.2751611350313e+116},
        {-1.2280760209282201e+66, -5.6334765238469545e+20, 2, 6.918337433398509e+86, 6.91833743339851e+86},
        {6.357995500901358e-124, -1.317484762865612e-131, 3, -48258588.48699183, -48258588.48699182},
        {4.487976006258921e-134, -8.20423364170612e+158, 0, -8.20423364170612e+158, -8.204233641706119e+158},
        {1.4366870856264788e+68, 8.423740531088334e+155, 1, -8.423740531088334e+155, -8.423740531088333e+155},
        {-7.288522672983534e+32, -8.27933867935607e+71, 2, 6.034414768179625e+104, 6.034414768179627e+104},
        {-3.2203343329787468e+150, -4.3538290944758115e+85, 3, 7.396556601325357e+64, 7.396556601325358e+64},
    };
    for (int i=0;i<64;++i) {
        const FloatingInterval a=FloatingInterval::exact(cases[i][0]);
        const FloatingInterval b=FloatingInterval::exact(cases[i][1]);
        FloatingInterval result;
        switch (static_cast<int>(cases[i][2])) {
        case 0: result=a.add(b); break;
        case 1: result=a.subtract(b); break;
        case 2: result=a.multiply(b); break;
        default: result=a.divide(b); break;
        }
        ASSERT_TRUE(result.isValid()) << i;
        EXPECT_LE(result.getLower(),cases[i][3]) << i;
        EXPECT_GE(result.getUpper(),cases[i][4]) << i;
    }
}

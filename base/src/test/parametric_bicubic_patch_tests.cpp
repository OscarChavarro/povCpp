#include <cmath>
#include <gtest/gtest.h>
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/curve/ParametricCurve.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/surface/ParametricBiCubicPatch.h"

/*
Exercises the ray intersection of bicubic patches. C++ counterpart of the
intersection tests of Java's `ParametricBiCubicPatchTest`.
*/
namespace {

const double EPS = 1.0e-9;

Vector3Dd evaluate(ParametricBiCubicPatch& patch, double s, double t)
{
    Vector3Dd p;
    patch.evaluate(p, s, t);
    return p;
}

void buildPlanarBezierPatch(ParametricBiCubicPatch& patch)
{
    Vector3Dd points[4][4];
    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            points[i][j] = Vector3Dd(i / 3.0, j / 3.0, 0);
        }
    }
    patch.buildBezierPatch(points);
}

void buildDomeNet(Vector3Dd points[4][4])
{
    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            bool inner = (i == 1 || i == 2) && (j == 1 || j == 2);
            points[i][j] = Vector3Dd(
                i / 3.0 + 0.1 * std::sin((double)j), j / 3.0, inner ? 1.2 : 0.2 * i);
        }
    }
}

void addContourPoint(ParametricCurve* contour, const Vector3Dd& a,
                     const Vector3Dd& b, const Vector3Dd& c)
{
    java::ArrayList<Vector3Dd> pointParameters;
    pointParameters.add(a);
    pointParameters.add(b);
    pointParameters.add(c);
    contour->addPoint(pointParameters, ParametricCurve::HERMITE);
}

/**
@return the contour of the patch created by the scene editor, owned by the
caller (the patch references it)
*/
ParametricCurve* buildEditorContour()
{
    ParametricCurve* contour = new ParametricCurve();

    addContourPoint(contour, Vector3Dd(0, 0, 0), Vector3Dd(0, -1, 0), Vector3Dd(1, 0, 0));
    addContourPoint(contour, Vector3Dd(1, 0, 0), Vector3Dd(1, 0, 0), Vector3Dd(0, 1, 0));
    addContourPoint(contour, Vector3Dd(1, 1, 0.4), Vector3Dd(0, 1, 0), Vector3Dd(-1, 0, 0));
    addContourPoint(contour, Vector3Dd(0, 1, 0), Vector3Dd(-1, 0, 0), Vector3Dd(0, -1, 0));
    java::ArrayList<Vector3Dd> firstPoint = contour->getPointVector(0);
    contour->addPoint(firstPoint, ParametricCurve::HERMITE);
    return contour;
}

}

TEST(ParametricBiCubicPatchTest, RayHittingThePlanarPatchInteriorReportsExactHitData) {
    // Arrange
    ParametricBiCubicPatch patch;
    buildPlanarBezierPatch(patch);
    Ray ray(Vector3Dd(0.3, 0.6, 5), Vector3Dd(0, 0, -1));
    RayHit hit;

    // Act
    bool intersects = patch.doIntersectionFirstHit(ray, &hit);

    // Assert
    ASSERT_TRUE(intersects);
    EXPECT_NEAR(hit.getHitDistance(), 5.0, EPS);
    EXPECT_LT(hit.point.subtract(Vector3Dd(0.3, 0.6, 0)).length(), EPS);
    EXPECT_NEAR(hit.normal.z(), 1.0, EPS);
    EXPECT_NEAR(hit.u, 0.3, EPS);
    EXPECT_NEAR(hit.v, 0.6, EPS);
    EXPECT_NEAR(hit.tangent.x(), 1.0, EPS);
}

TEST(ParametricBiCubicPatchTest, RayMissingOrPointingAwayFromThePlanarPatchReportsNoHit) {
    // Arrange
    ParametricBiCubicPatch patch;
    buildPlanarBezierPatch(patch);
    Ray outside(Vector3Dd(1.5, 0.5, 5), Vector3Dd(0, 0, -1));
    Ray away(Vector3Dd(0.5, 0.5, 5), Vector3Dd(0, 0, 1));
    Ray parallel(Vector3Dd(-1, 0.5, 1), Vector3Dd(1, 0, 0));
    RayHit hit;

    // Act / Assert
    EXPECT_FALSE(patch.doIntersectionFirstHit(outside, &hit));
    EXPECT_FALSE(patch.doIntersectionFirstHit(away, &hit));
    EXPECT_FALSE(patch.doIntersectionFirstHit(parallel, &hit));
    EXPECT_EQ(patch.doIntersectionFirstHit(outside), nullptr);
}

TEST(ParametricBiCubicPatchTest, BezierPatchReturnsItsControlPointsAsBezierNet) {
    // Arrange
    Vector3Dd points[4][4];
    buildDomeNet(points);
    ParametricBiCubicPatch patch;
    patch.buildBezierPatch(points);

    // Act / Assert
    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            Vector3Dd controlPoint;
            ASSERT_TRUE(patch.getBezierControlPoint(i, j, controlPoint));
            EXPECT_LT(controlPoint.subtract(points[i][j]).length(), EPS);
        }
    }
}

TEST(ParametricBiCubicPatchTest, RaysAimedAtSurfacePointsOfCurvedPatchesHitTheSurface) {
    // Arrange
    Vector3Dd domeNet[4][4];
    buildDomeNet(domeNet);
    ParametricBiCubicPatch dome;
    dome.buildBezierPatch(domeNet);
    ParametricCurve* contour = buildEditorContour();
    ParametricBiCubicPatch ferguson;
    ferguson.buildFergusonPatch(contour);
    ParametricBiCubicPatch* patches[] = { &dome, &ferguson };
    Vector3Dd eyes[] = {
        Vector3Dd(0.3, -0.2, 4), Vector3Dd(-3, 2, 1.5), Vector3Dd(2, 3, -2)
    };

    for ( int k = 0; k < 2; k++ ) {
        ParametricBiCubicPatch& patch = *patches[k];
        for ( int e = 0; e < 3; e++ ) {
            const Vector3Dd& eye = eyes[e];
            for ( int a = 1; a < 10; a++ ) {
                for ( int b = 1; b < 10; b++ ) {
                    Vector3Dd target = evaluate(patch, a / 10.0, b / 10.0);
                    Vector3Dd direction = target.subtract(eye).normalized();
                    double targetDistance = target.subtract(eye).length();
                    RayHit hit;

                    // Act
                    bool intersects = patch.doIntersectionFirstHit(
                        Ray(eye, direction), &hit);

                    // Assert: the nearest hit is on the surface, not
                    // beyond the aimed point, and its (u, v) are coherent
                    ASSERT_TRUE(intersects);
                    EXPECT_LE(hit.getHitDistance(), targetDistance + 1.0e-6);
                    Vector3Dd onSurface = evaluate(patch, hit.u, hit.v);
                    EXPECT_LT(onSurface.subtract(hit.point).length(), 1.0e-6);
                    EXPECT_LE(hit.normal.dotProduct(direction), 0.0);
                }
            }
        }
    }
    delete contour;
}

TEST(ParametricBiCubicPatchTest, RayLeavingFromTheSurfaceDoesNotHitItself) {
    // Arrange: shadow rays as built by LightingShader, with an origin
    // offset of VSDK.EPSILON along the light direction
    Vector3Dd domeNet[4][4];
    buildDomeNet(domeNet);
    ParametricBiCubicPatch dome;
    dome.buildBezierPatch(domeNet);
    Vector3Dd lightDirection = Vector3Dd(-0.1, 0.05, 1).normalized();

    for ( int a = 0; a <= 20; a++ ) {
        for ( int b = 0; b <= 20; b++ ) {
            Vector3Dd p = evaluate(dome, a / 20.0, b / 20.0);
            Vector3Dd origin = p.add(lightDirection.multiply(1.0e-6));
            RayHit hit(RayHit::DETAIL_NONE, false);

            // Act
            bool intersects = dome.doIntersectionFirstHit(
                Ray(origin, lightDirection), &hit);

            // Assert
            EXPECT_FALSE(intersects) << "self hit from (" << a << ", " << b
                << ") at distance " << hit.getHitDistance();
        }
    }
}

TEST(ParametricBiCubicPatchTest, MinMaxOfFergusonPatchContainsTheSampledSurface) {
    // Arrange
    ParametricCurve* contour = buildEditorContour();
    ParametricBiCubicPatch patch;
    patch.buildFergusonPatch(contour);

    // Act
    double* minMax = patch.getMinMax();

    // Assert
    for ( int a = 0; a <= 20; a++ ) {
        for ( int b = 0; b <= 20; b++ ) {
            Vector3Dd p = evaluate(patch, a / 20.0, b / 20.0);
            EXPECT_GE(p.x(), minMax[0] - EPS);
            EXPECT_LE(p.x(), minMax[3] + EPS);
            EXPECT_GE(p.y(), minMax[1] - EPS);
            EXPECT_LE(p.y(), minMax[4] + EPS);
            EXPECT_GE(p.z(), minMax[2] - EPS);
            EXPECT_LE(p.z(), minMax[5] + EPS);
        }
    }
    delete[] minMax;
    delete contour;
}

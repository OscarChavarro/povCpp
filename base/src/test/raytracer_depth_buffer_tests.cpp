#include "java/lang/Double.h"
#include <gtest/gtest.h>
#include <cmath>
#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector4Dd.h"
#include "vsdk/toolkit/environment/background/SimpleBackground.h"
#include "vsdk/toolkit/environment/camera/Camera.h"
#include "vsdk/toolkit/environment/camera/CameraSnapshot.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/light/PointLight.h"
#include "vsdk/toolkit/environment/material/RendererConfiguration.h"
#include "vsdk/toolkit/environment/material/SimpleMaterial.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/scene/SimpleScene.h"
#include "vsdk/toolkit/environment/scene/SimpleSceneSnapshot.h"
#include "vsdk/toolkit/media/RGBImageUncompressed.h"
#include "vsdk/toolkit/media/ZBuffer.h"
#include "vsdk/toolkit/render/raytracing/DepthBufferEncoder.h"
#include "vsdk/toolkit/render/raytracing/DepthBufferMode.h"
#include "vsdk/toolkit/render/raytracing/ParallelRaytracer.h"
#include "vsdk/toolkit/render/raytracing/SimpleRaytracer.h"

// C++ counterpart of Java's `vsdk.toolkit.render.RaytracerDepthBufferTest`:
// checks the depth buffers exported by the raytracers against the OpenGL
// projection of the same camera (`Camera::calculateProjectionMatrix`).

static const int WIDTH = 81;
static const int HEIGHT = 57;

static Camera* fillScene(SimpleScene* scene, int projectionMode)
{
    Camera* camera = new Camera();
    SimpleBackground* background = new SimpleBackground();
    SimpleBody* body = new SimpleBody();
    SimpleMaterial material;

    camera->setPosition(Vector3Dd(-5, -5, 5));
    camera->setRotation(Matrix4x4d().eulerAnglesRotation(
        java::Math::toRadians(45), java::Math::toRadians(-35), 0));
    camera->setNearPlaneDistance(0.5);
    camera->setFarPlaneDistance(30);
    camera->setProjectionMode(projectionMode);
    camera->setOrthogonalZoom(0.6);
    camera->updateViewportResize(WIDTH, HEIGHT);
    background->setColor(0.2, 0.3, 0.4);

    body->setGeometry(new Sphere(1.0));
    body->setPosition(Vector3Dd(0, 0, 0));
    body->setRotation(Matrix4x4d());
    body->setRotationInverse(Matrix4x4d());
    body->setMaterial(new SimpleMaterial(material.withDiffuse(ColorRgb(0.8, 0.5, 0.3))));

    scene->addCamera(camera);
    scene->addBackground(background);
    scene->addBody(body);
    scene->addLight(new PointLight(Vector3Dd(3, -3, 4), ColorRgb(1, 1, 1)));
    return camera;
}

static void raytrace(SimpleSceneSnapshot* snapshot, DepthBufferMode mode, ZBuffer* depth)
{
    RGBImageUncompressed image;
    RendererConfiguration quality;
    SimpleRaytracer raytracer;

    image.init(WIDTH, HEIGHT);
    raytracer.execute(&image, &quality, snapshot, 0, depth, mode, 0, 0, WIDTH, HEIGHT);
}

static Vector3Dd unproject(const Matrix4x4d& inverseProjection, int x, int y, double windowDepth)
{
    double ndcX = 2.0 * (x + 0.5) / WIDTH - 1.0;
    double ndcY = 1.0 - 2.0 * (y + 0.5) / HEIGHT;
    double ndcZ = 2.0 * windowDepth - 1.0;
    Vector4Dd p = inverseProjection.multiply(Vector4Dd(ndcX, ndcY, ndcZ, 1.0)).dividedByW();
    return Vector3Dd(p.x(), p.y(), p.z());
}

static void checkOpenGlDepthMatchesProjection(int projectionMode)
{
    // Arrange
    SimpleScene scene;
    Camera* camera = fillScene(&scene, projectionMode);
    SimpleSceneSnapshot* snapshot = scene.exportToSimpleSceneSnapshot(WIDTH, HEIGHT);
    Matrix4x4d inverseProjection = camera->calculateProjectionMatrix().invert();
    CameraSnapshot* cameraSnapshot = snapshot->getCameraSnapshot();
    ZBuffer glDepth(WIDTH, HEIGHT);
    ZBuffer distance(WIDTH, HEIGHT);

    // Action
    raytrace(snapshot, DepthBufferMode::OPENGL_DEPTH, &glDepth);
    raytrace(snapshot, DepthBufferMode::RAY_DISTANCE, &distance);

    // Assert
    int hits = 0;
    int misses = 0;
    for ( int y = 0; y < HEIGHT; y++ ) {
        for ( int x = 0; x < WIDTH; x++ ) {
            float d = glDepth.getDepth(x, y);
            float t = distance.getDepth(x, y);
            if ( std::isinf(t) ) {
                misses++;
                EXPECT_EQ(d, 1.0f);
                continue;
            }
            hits++;
            EXPECT_GE(d, 0.0f);
            EXPECT_LE(d, 1.0f);
            Vector3Dd p = unproject(inverseProjection, x, y, d);
            // The point is on the sphere...
            EXPECT_NEAR(p.length(), 1.0, 2e-3);
            // ... at the ray distance (from the eye, or from the image plane
            // in orthogonal projection)
            double expectedDistance;
            if ( projectionMode == Camera::PROJECTION_MODE_ORTHOGONAL ) {
                expectedDistance = p.subtract(cameraSnapshot->getEyePosition())
                    .dotProduct(cameraSnapshot->getFront());
            }
            else {
                expectedDistance = p.subtract(cameraSnapshot->getEyePosition()).length();
            }
            EXPECT_NEAR((double)t, expectedDistance, 2e-3);
        }
    }
    EXPECT_GT(hits, 100);
    EXPECT_GT(misses, 200);

    delete snapshot;
}

TEST(RaytracerDepthBufferTest, given_perspectiveCamera_when_exportingOpenGlDepth_then_unprojectsToHitSurface) {
    checkOpenGlDepthMatchesProjection(Camera::PROJECTION_MODE_PERSPECTIVE);
}

TEST(RaytracerDepthBufferTest, given_orthogonalCamera_when_exportingOpenGlDepth_then_unprojectsToHitSurface) {
    checkOpenGlDepthMatchesProjection(Camera::PROJECTION_MODE_ORTHOGONAL);
}

TEST(RaytracerDepthBufferTest, given_depthModeNone_when_raytracing_then_depthBufferIsUntouched) {
    // Arrange
    SimpleScene scene;
    fillScene(&scene, Camera::PROJECTION_MODE_PERSPECTIVE);
    SimpleSceneSnapshot* snapshot = scene.exportToSimpleSceneSnapshot(WIDTH, HEIGHT);
    ZBuffer depth(WIDTH, HEIGHT);

    // Action
    raytrace(snapshot, DepthBufferMode::NONE, &depth);

    // Assert
    for ( int i = 0; i < WIDTH * HEIGHT; i++ ) {
        EXPECT_EQ(depth.getZBuffer()[i], 0.0f);
    }
    delete snapshot;
}

TEST(RaytracerDepthBufferTest, given_depthMode_when_raytracingInParallel_then_depthEqualsSerial) {
    // Arrange
    SimpleScene scene;
    fillScene(&scene, Camera::PROJECTION_MODE_PERSPECTIVE);
    SimpleSceneSnapshot* snapshot = scene.exportToSimpleSceneSnapshot(WIDTH, HEIGHT);
    ParallelRaytracer parallel(3);
    RendererConfiguration quality;
    RGBImageUncompressed image;
    image.init(WIDTH, HEIGHT);

    // Action / Assert
    EXPECT_EQ(parallel.getDepthBufferMode(), DepthBufferMode::NONE);
    parallel.execute(&image, &quality, snapshot, false);
    EXPECT_EQ(parallel.getDepthBuffer(), (ZBuffer*)0);

    DepthBufferMode modes[] = { DepthBufferMode::RAY_DISTANCE, DepthBufferMode::OPENGL_DEPTH };
    for ( DepthBufferMode mode : modes ) {
        ZBuffer serial(WIDTH, HEIGHT);
        raytrace(snapshot, mode, &serial);
        parallel.setDepthBufferMode(mode);
        parallel.execute(&image, &quality, snapshot, false);
        ASSERT_NE(parallel.getDepthBuffer(), (ZBuffer*)0);
        EXPECT_EQ(parallel.getDepthBuffer()->getXSize(), WIDTH);
        EXPECT_EQ(parallel.getDepthBuffer()->getYSize(), HEIGHT);
        for ( int i = 0; i < WIDTH * HEIGHT; i++ ) {
            float expected = serial.getZBuffer()[i];
            float actual = parallel.getDepthBuffer()->getZBuffer()[i];
            EXPECT_TRUE(expected == actual) << "pixel " << i;
        }
    }
    parallel.setDepthBufferMode(DepthBufferMode::NONE);
    EXPECT_EQ(parallel.getDepthBuffer(), (ZBuffer*)0);
    parallel.dispose();
    delete snapshot;
}

TEST(RaytracerDepthBufferTest, given_customDepthRange_when_raytracingInParallel_then_depthIsRemapped) {
    // Arrange
    SimpleScene scene;
    fillScene(&scene, Camera::PROJECTION_MODE_PERSPECTIVE);
    SimpleSceneSnapshot* snapshot = scene.exportToSimpleSceneSnapshot(WIDTH, HEIGHT);
    ParallelRaytracer parallel(2);
    RendererConfiguration quality;
    RGBImageUncompressed image;
    ZBuffer standard(WIDTH, HEIGHT);
    raytrace(snapshot, DepthBufferMode::OPENGL_DEPTH, &standard);
    image.init(WIDTH, HEIGHT);
    parallel.setDepthBufferMode(DepthBufferMode::OPENGL_DEPTH);
    parallel.setOpenGlDepthRange(0.25, 0.75);

    // Action
    parallel.execute(&image, &quality, snapshot, false);

    // Assert
    for ( int i = 0; i < WIDTH * HEIGHT; i++ ) {
        EXPECT_NEAR((double)parallel.getDepthBuffer()->getZBuffer()[i],
            0.25 + 0.5 * standard.getZBuffer()[i], 1e-6);
    }
    parallel.dispose();
    delete snapshot;
}

TEST(RaytracerDepthBufferTest, given_encoder_when_convertingPlaneDistances_then_matchesOpenGlLimits) {
    // Arrange
    SimpleScene scene;
    Camera* camera = fillScene(&scene, Camera::PROJECTION_MODE_PERSPECTIVE);
    CameraSnapshot* perspective = camera->exportToCameraSnapshot(WIDTH, HEIGHT);
    camera->setProjectionMode(Camera::PROJECTION_MODE_ORTHOGONAL);
    CameraSnapshot* orthogonal = camera->exportToCameraSnapshot(WIDTH, HEIGHT);
    CameraSnapshot* snapshots[] = { perspective, orthogonal };

    for ( CameraSnapshot* snapshot : snapshots ) {
        // Action
        DepthBufferEncoder encoder(DepthBufferMode::OPENGL_DEPTH, snapshot);
        DepthBufferEncoder identity(DepthBufferMode::RAY_DISTANCE, snapshot);

        // Assert
        EXPECT_NEAR(encoder.eyeDepthToWindowDepth(0.5), 0.0, 1e-12);
        EXPECT_NEAR(encoder.eyeDepthToWindowDepth(30), 1.0, 1e-12);
        EXPECT_EQ(encoder.eyeDepthToWindowDepth(0.1), 0.0);
        EXPECT_EQ(encoder.eyeDepthToWindowDepth(100), 1.0);
        EXPECT_LT(encoder.eyeDepthToWindowDepth(5), encoder.eyeDepthToWindowDepth(6));
        EXPECT_EQ(encoder.encode(snapshot->getEyePosition(), snapshot->getFront(),
            java::Double::POSITIVE_INFINITY), 1.0f);
        EXPECT_EQ(identity.encode(snapshot->getEyePosition(), snapshot->getFront(), 7.25), 7.25f);
    }
    delete perspective;
    delete orthogonal;
}

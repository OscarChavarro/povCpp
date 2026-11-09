#include <gtest/gtest.h>
#include <unistd.h>
#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/background/SimpleBackground.h"
#include "vsdk/toolkit/environment/camera/Camera.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/light/PointLight.h"
#include "vsdk/toolkit/environment/material/RendererConfiguration.h"
#include "vsdk/toolkit/environment/material/SimpleMaterial.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/scene/SimpleScene.h"
#include "vsdk/toolkit/environment/scene/SimpleSceneSnapshot.h"
#include "vsdk/toolkit/media/RGBImageUncompressed.h"
#include "vsdk/toolkit/media/RGBPixel.h"
#include "vsdk/toolkit/render/raytracing/ParallelRaytracer.h"
#include "vsdk/toolkit/render/raytracing/SimpleRaytracer.h"

// C++ counterpart of Java's `vsdk.toolkit.render.ParallelRaytracerTest`

static SimpleBody* createBody(Geometry* geometry, const Vector3Dd& position)
{
    SimpleBody* body = new SimpleBody();
    SimpleMaterial material;

    body->setGeometry(geometry);
    body->setPosition(position);
    body->setRotation(Matrix4x4d());
    body->setRotationInverse(Matrix4x4d());
    body->setMaterial(new SimpleMaterial(material.withDiffuse(ColorRgb(0.8, 0.5, 0.3))));
    return body;
}

static void fillScene(SimpleScene* scene)
{
    Camera* camera = new Camera();
    SimpleBackground* background = new SimpleBackground();

    camera->setPosition(Vector3Dd(-5, -5, 5));
    camera->setRotation(Matrix4x4d().eulerAnglesRotation(
        java::Math::toRadians(45), java::Math::toRadians(-35), 0));
    background->setColor(0.2, 0.3, 0.4);
    scene->addCamera(camera);
    scene->addBackground(background);
    scene->addBody(createBody(new Sphere(1.0), Vector3Dd(0, 0, 0)));
    scene->addBody(createBody(new Box(1, 2, 1), Vector3Dd(1.5, 0, 0)));
    scene->addLight(new PointLight(Vector3Dd(3, -3, 4), ColorRgb(1, 1, 1)));
}

static bool samePixel(const RGBPixel& a, const RGBPixel& b)
{
    return a.getR() == b.getR() && a.getG() == b.getG() && a.getB() == b.getB();
}

TEST(ParallelRaytracerTest, given_scene_when_raytracingInParallel_then_imageEqualsSerialRaytracing) {
    // Arrange
    int width = 97;
    int height = 61;
    SimpleScene scene;
    fillScene(&scene);
    SimpleSceneSnapshot* snapshot = scene.exportToSimpleSceneSnapshot(width, height);
    RendererConfiguration quality;
    RGBImageUncompressed serialImage;
    RGBImageUncompressed parallelImage;
    ParallelRaytracer parallelRaytracer(4);
    serialImage.init(width, height);
    parallelImage.init(width, height);

    // Action
    SimpleRaytracer serialRaytracer;
    serialRaytracer.execute(&serialImage, &quality, snapshot, 0, 0);
    parallelRaytracer.execute(&parallelImage, &quality, snapshot, false);
    // Threads are reused between images
    parallelRaytracer.execute(&parallelImage, &quality, snapshot, false);
    parallelRaytracer.dispose();

    // Assert
    int differentPixels = 0;
    int objectPixels = 0;
    RGBPixel backgroundPixel;
    RGBPixel expected;
    RGBPixel actual;
    serialImage.getPixelRgb(0, 0, &backgroundPixel);
    for ( int y = 0; y < height; y++ ) {
        for ( int x = 0; x < width; x++ ) {
            serialImage.getPixelRgb(x, y, &expected);
            parallelImage.getPixelRgb(x, y, &actual);
            if ( !samePixel(expected, actual) ) {
                differentPixels++;
            }
            if ( !samePixel(expected, backgroundPixel) ) {
                objectPixels++;
            }
        }
    }
    EXPECT_GT(objectPixels, 100);
    EXPECT_EQ(differentPixels, 0);

    delete snapshot;
}

TEST(ParallelRaytracerTest, given_defaultConstructor_when_created_then_usesOneThreadPerProcessor) {
    // Arrange / Action
    ParallelRaytracer raytracer;

    // Assert
#ifdef VITRAL_WITH_POSIX_THREADS
    EXPECT_EQ(raytracer.getNumberOfThreads(), (int)sysconf(_SC_NPROCESSORS_ONLN));
#else
    EXPECT_EQ(raytracer.getNumberOfThreads(), 1);
#endif
}

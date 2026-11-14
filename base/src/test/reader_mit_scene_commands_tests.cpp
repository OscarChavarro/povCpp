#include <cmath>
#include <cstdio>
#include <string>
#include <gtest/gtest.h>
#include "java/lang/Exception.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/background/CubemapBackground.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/surface/QuadMesh.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/Cone.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/volume/Torus.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/scene/SimpleScene.h"
#include "vsdk/toolkit/io/geometry/ReaderMitScene.h"

TEST(ReaderMitCommands, ClassicPrimitivesRotationAndCubemapAreRead) {
    SimpleScene scene;
    ReaderMitScene reader;
#ifndef VITRAL_WITH_JPEG
    // Without a JPEG decoder the cubemap faces cannot be read.
    EXPECT_THROW(reader.importEnvironment(
        VITRAL_REPOSITORY_ROOT "/etc/geometry/mitscenes/object.ray", &scene), java::Exception);
    GTEST_SKIP() << "built without JPEG support";
#endif
    reader.importEnvironment(VITRAL_REPOSITORY_ROOT "/etc/geometry/mitscenes/object.ray", &scene);
    java::ArrayList<SimpleBody*>& bodies = scene.getSimpleBodies();
    ASSERT_EQ(12, bodies.size());
    const Box* cube = dynamic_cast<const Box*>(bodies[1]->getGeometry());
    ASSERT_NE(nullptr, cube);
    EXPECT_DOUBLE_EQ(1.0, cube->getSize().x());
    EXPECT_DOUBLE_EQ(-2.0, bodies[1]->getPosition().x());
    EXPECT_NE(nullptr, dynamic_cast<const Cone*>(bodies[2]->getGeometry()));
    // `rotation 0 0.3 0` turns every later primitive about its own origin.
    EXPECT_GT(std::fabs(bodies[1]->getRotation().get(0, 2)) + std::fabs(bodies[1]->getRotation().get(2, 0)), 0.1);
    EXPECT_NE(nullptr, dynamic_cast<CubemapBackground*>(scene.getActiveBackground()));
    // The six cubemap faces are scene resources.
    EXPECT_EQ(6, scene.getImages().size());
}

TEST(ReaderMitCommands, MeshPolyhedralAndTorusBodiesHaveRealHits) {
    const char* scenes[3] = {"quadmesh_test.ray", "balls_featured_object.ray", "balls_torus.ray"};
    for (int i = 0; i < 3; ++i) {
        SimpleScene scene;
        ReaderMitScene reader;
        const std::string path = std::string(VITRAL_REPOSITORY_ROOT "/etc/geometry/mitscenes/") + scenes[i];
        reader.importEnvironment(path.c_str(), &scene);
        java::ArrayList<SimpleBody*>& bodies = scene.getSimpleBodies();
        ASSERT_LE(2, bodies.size()) << scenes[i];
        SimpleBody* center = bodies[1];
        if (i == 0) EXPECT_NE(nullptr, dynamic_cast<const QuadMesh*>(center->getGeometry()));
        if (i == 1) EXPECT_NE(nullptr, dynamic_cast<const PolyhedralBoundedSolid*>(center->getGeometry()));
        if (i == 2) EXPECT_NE(nullptr, dynamic_cast<const Torus*>(center->getGeometry()));
        // Every object sits at the origin and is hit from above.
        const double height = i == 2 ? 0.13 : 0.0;
        RayHit hit(RayHit::DETAIL_POINT, false);
        const Vector3Dd origin = i == 2 ? Vector3Dd(0.36, 0, 5) : Vector3Dd(0, 0, 5);
        ASSERT_TRUE(center->doIntersectionFirstHit(Ray(origin, Vector3Dd(0, 0, -1)), &hit)) << scenes[i];
        EXPECT_LT(hit.getHitDistance(), 5.0) << scenes[i];
        if (i == 2) EXPECT_NEAR(5 - height, hit.getHitDistance(), 1e-6);
    }
}

TEST(ReaderMitCommands, TexturesAndBumpMapsAreSharedSceneResources) {
    const std::string path = testing::TempDir() + "reader_mit_textures.ray";
    FILE* file = fopen(path.c_str(), "w");
    ASSERT_NE(nullptr, file);
    fputs("eye 0 -5 0\nup 0 0 1\nlookat 0 0 0\n"
          "texture \"" VITRAL_REPOSITORY_ROOT "/etc/textures/miniearth.png\"\n"
          "bumpmap \"" VITRAL_REPOSITORY_ROOT "/etc/bumpmaps/earth.bw\" 1 1 0.2\n"
          "sphere -2 0 0 1\nsphere 2 0 0 1\n"
          "texture \"" VITRAL_REPOSITORY_ROOT "/etc/textures/miniearth.png\"\n"
          "notexture\nnobumpmap\nsphere 0 0 2 1\n", file);
    fclose(file);
    SimpleScene scene;
    ReaderMitScene reader;
    reader.importEnvironment(path.c_str(), &scene);
    std::remove(path.c_str());
    java::ArrayList<SimpleBody*>& bodies = scene.getSimpleBodies();
    ASSERT_EQ(3, bodies.size());
    ASSERT_NE(nullptr, bodies[0]->getTexture());
    EXPECT_EQ(bodies[0]->getTexture(), bodies[1]->getTexture());
    ASSERT_NE(nullptr, bodies[0]->getNormalMap());
    EXPECT_EQ(bodies[0]->getNormalMap(), bodies[1]->getNormalMap());
    EXPECT_EQ(nullptr, bodies[2]->getTexture());
    EXPECT_EQ(nullptr, bodies[2]->getNormalMap());
    // A repeated texture file is loaded once.
    EXPECT_EQ(1, scene.getImages().size());
    EXPECT_EQ(1, scene.getNormalMaps().size());
    // Removing a body leaves the shared resources to the scene.
    delete bodies[0];
    bodies.remove((long int)0);
    EXPECT_EQ(scene.getImages()[0], bodies[0]->getTexture());
}

TEST(ReaderMitCommands, ResourcesResolveFromTheSceneDirectory) {
    SimpleScene scene;
    ReaderMitScene reader;
    // bumpMapping.ray names its files relative to its own directory.
    reader.importEnvironment(VITRAL_REPOSITORY_ROOT "/etc/geometry/mitscenes/bumpMapping.ray", &scene);
    ASSERT_EQ(1, scene.getSimpleBodies().size());
    EXPECT_NE(nullptr, scene.getSimpleBodies()[0]->getTexture());
    EXPECT_NE(nullptr, scene.getSimpleBodies()[0]->getNormalMap());
}

TEST(ReaderMitCommands, UnknownAndMalformedCommandsAreRejected) {
    const char* bodies[10] = {"spheer 0 0 0 1\n", "cube 0 0 0 -1\n", "texture /nonexistent/file.png\n",
        "quads\nv 0 0 0\nq 0 1 2 3\nend\n", "rotation 1 2\n", "v 0 0 0\n",
        "light 1 1 1 spot 0 0 0\n", "appel 4 0 0 0\n", "quads\nv 0 0 0\n", "csg union\ntorus 0 0 0 1 0.2\nend\n"};
    const std::string path = testing::TempDir() + "reader_mit_commands_malformed.ray";
    for (int i = 0; i < 10; ++i) {
        FILE* file = fopen(path.c_str(), "w");
        ASSERT_NE(nullptr, file);
        fputs("eye 0 -5 0\nup 0 0 1\nlookat 0 0 0\n", file);
        fputs(bodies[i], file);
        fclose(file);
        SimpleScene scene;
        ReaderMitScene reader;
        EXPECT_THROW(reader.importEnvironment(path.c_str(), &scene), java::Exception) << i;
    }
    std::remove(path.c_str());
}

#include "java/lang/Math.h"
#include <cmath>
#include <gtest/gtest.h>
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/render/SpherePolyhedralCache.h"

TEST(SpherePolyhedralCacheTest, EveryFaceIsAnOutwardTriangleOnTheSphere) {
    // Arrange
    Sphere sphere(2.0);

    // Act
    const SpherePolyhedralCache::Entry* entry = SpherePolyhedralCache::obtain(&sphere, 32, 15);

    // Assert
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->getVertexCount(), 3 * (int)entry->getSolid()->getPolygonsList().size());
    const java::ArrayList<float>& p = entry->getPositions();
    const java::ArrayList<float>& n = entry->getNormals();
    for (int i = 0; i < entry->getVertexCount(); i++) {
        double length = std::sqrt(p[3*i]*p[3*i] + p[3*i+1]*p[3*i+1] + p[3*i+2]*p[3*i+2]);

        EXPECT_NEAR(length, 2.0, 1e-5);
        // The normal of the parametric surface is the radial direction
        EXPECT_NEAR(n[3*i], p[3*i] / length, 1e-5);
        EXPECT_NEAR(n[3*i+1], p[3*i+1] / length, 1e-5);
        EXPECT_NEAR(n[3*i+2], p[3*i+2] / length, 1e-5);
    }
    for (int t = 0; t < entry->getVertexCount(); t += 3) {
        double ux = p[3*t+3] - p[3*t];
        double uy = p[3*t+4] - p[3*t+1];
        double uz = p[3*t+5] - p[3*t+2];
        double vx = p[3*t+6] - p[3*t];
        double vy = p[3*t+7] - p[3*t+1];
        double vz = p[3*t+8] - p[3*t+2];
        double outward = (uy*vz - uz*vy) * p[3*t] + (uz*vx - ux*vz) * p[3*t+1] +
            (ux*vy - uy*vx) * p[3*t+2];

        EXPECT_GT(outward, 0.0);
    }
}

TEST(SpherePolyhedralCacheTest, TextureCoordinatesOfATriangleDoNotJumpAcrossTheSeam) {
    // Arrange
    Sphere sphere(1.0);
    const SpherePolyhedralCache::Entry* entry = SpherePolyhedralCache::obtain(&sphere, 16, 8);
    const java::ArrayList<float>& uv = entry->getUvs();

    // Act / Assert
    for (int t = 0; t < entry->getVertexCount(); t += 3) {
        float min = java::Math::min(uv[2*t], java::Math::min(uv[2*t+2], uv[2*t+4]));
        float max = java::Math::max(uv[2*t], java::Math::max(uv[2*t+2], uv[2*t+4]));

        EXPECT_LE(max - min, 1.0f / 16 + 1e-5f);
    }
}

TEST(SpherePolyhedralCacheTest, SpheresOfTheSameRadiusShareTheEntry) {
    // Arrange
    Sphere first(0.5);
    Sphere second(0.5);

    // Act
    const SpherePolyhedralCache::Entry* a = SpherePolyhedralCache::obtain(&first, 20, 10);
    const SpherePolyhedralCache::Entry* b = SpherePolyhedralCache::obtain(&second, 20, 10);

    // Assert
    EXPECT_EQ(a, b);
    EXPECT_EQ(a->getSolid(), b->getSolid());
}

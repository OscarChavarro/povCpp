#include "java/lang/Double.h"
#include "java/lang/Math.h"
#include <cmath>
#include <gtest/gtest.h>
#include "java/util/Random.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/volume/VoxelVolume.h"

/*
Exercises the intersection of rays with the filled voxels of a volume (the
cube <-1, -1, -1>-<1, 1, 1>), traversed voxel by voxel. C++ counterpart of
Java's `VoxelVolumeTest`.
*/
namespace {

const double EPS = 1.0e-9;
const char FILLED = (char)-1;

void fill(VoxelVolume& volume)
{
    for ( int z = 0; z < volume.getZSize(); z++ ) {
        for ( int y = 0; y < volume.getYSize(); y++ ) {
            for ( int x = 0; x < volume.getXSize(); x++ ) {
                volume.putVoxel(x, y, z, FILLED);
            }
        }
    }
}

Vector3Dd randomDirection(java::Random& random)
{
    Vector3Dd v;
    do {
        v = Vector3Dd(random.nextDouble() * 2 - 1,
            random.nextDouble() * 2 - 1, random.nextDouble() * 2 - 1);
    } while ( v.length() < 0.1 || v.length() > 1 );
    return v.normalized();
}

/**
@return the smallest entry distance of the ray into the box of a filled
voxel, or infinity
*/
double bruteForceFirstHit(const VoxelVolume& volume,
    const Vector3Dd& origin, const Vector3Dd& direction)
{
    const double infinity = java::Double::POSITIVE_INFINITY;
    double best = infinity;
    int n = volume.getXSize();
    double cell = 2.0 / n;
    double o[3] = { origin.x(), origin.y(), origin.z() };
    double d[3] = { direction.x(), direction.y(), direction.z() };

    for ( int z = 0; z < n; z++ ) {
        for ( int y = 0; y < n; y++ ) {
            for ( int x = 0; x < n; x++ ) {
                if ( !volume.isFilled(x, y, z) ) {
                    continue;
                }
                int g[3] = { x, y, z };
                double tNear = -infinity;
                double tFar = infinity;
                for ( int a = 0; a < 3; a++ ) {
                    double lo = -1 + g[a] * cell;
                    double hi = lo + cell;
                    if ( std::abs(d[a]) < 1e-15 ) {
                        if ( o[a] < lo || o[a] > hi ) {
                            tNear = infinity;
                        }
                        continue;
                    }
                    double t1 = (lo - o[a]) / d[a];
                    double t2 = (hi - o[a]) / d[a];
                    tNear = java::Math::max(tNear, java::Math::min(t1, t2));
                    tFar = java::Math::min(tFar, java::Math::max(t1, t2));
                }
                if ( tNear <= tFar && tNear >= 0 && tNear < best ) {
                    best = tNear;
                }
            }
        }
    }
    return best;
}

}

TEST(VoxelVolumeTest, RayEnteringTheTopFaceOfOneFilledVoxelHitsItWithItsNormal) {
    // Arrange: in a 4^3 volume the voxel (1, 1, 1) spans [-0.5, 0]^3
    VoxelVolume volume;
    volume.init(4, 4, 4);
    volume.putVoxel(1, 1, 1, FILLED);
    Ray ray(Vector3Dd(-0.25, -0.25, 5), Vector3Dd(0, 0, -1));
    RayHit hit(RayHit::DETAIL_ALL, true);

    // Act
    bool intersects = volume.doIntersectionFirstHit(ray, &hit);

    // Assert
    ASSERT_TRUE(intersects);
    EXPECT_NEAR(hit.getRay()->getT(), 5.0, EPS);
    EXPECT_NEAR(hit.normal.x(), 0.0, EPS);
    EXPECT_NEAR(hit.normal.y(), 0.0, EPS);
    EXPECT_NEAR(hit.normal.z(), 1.0, EPS);
    EXPECT_NEAR(hit.point.z(), 0.0, EPS);
}

TEST(VoxelVolumeTest, RayPassingBesideOneFilledVoxelMissesTheVolume) {
    // Arrange
    VoxelVolume volume;
    volume.init(4, 4, 4);
    volume.putVoxel(1, 1, 1, FILLED);
    Ray ray(Vector3Dd(0.25, -0.25, 5), Vector3Dd(0, 0, -1));
    RayHit hit;

    // Act & Assert
    EXPECT_FALSE(volume.doIntersectionFirstHit(ray, &hit));
}

TEST(VoxelVolumeTest, RayFromOutsideHitsTheFaceOfAFullVolumeCube) {
    // Arrange
    VoxelVolume volume;
    volume.init(8, 8, 8);
    fill(volume);
    Ray ray(Vector3Dd(-5, 0.3, -0.2), Vector3Dd(1, 0, 0));
    RayHit hit(RayHit::DETAIL_ALL, true);

    // Act
    bool intersects = volume.doIntersectionFirstHit(ray, &hit);

    // Assert
    ASSERT_TRUE(intersects);
    EXPECT_NEAR(hit.getRay()->getT(), 4.0, EPS);
    EXPECT_NEAR(hit.normal.x(), -1.0, EPS);
}

TEST(VoxelVolumeTest, RayStartingInsideFilledVoxelsHitsOnlyTheEnteredOnes) {
    // Arrange: two filled voxels of the row y = z = 0 of a 4^3 volume,
    // x = 0 spans [-1, -0.5] and x = 3 spans [0.5, 1]
    VoxelVolume volume;
    volume.init(4, 4, 4);
    volume.putVoxel(0, 0, 0, FILLED);
    volume.putVoxel(3, 0, 0, FILLED);
    Ray ray(Vector3Dd(-0.75, -0.75, -0.75), Vector3Dd(1, 0, 0));
    RayHit hit(RayHit::DETAIL_ALL, true);

    // Act
    bool intersects = volume.doIntersectionFirstHit(ray, &hit);

    // Assert
    ASSERT_TRUE(intersects);
    EXPECT_NEAR(hit.getRay()->getT(), 1.25, EPS);
    EXPECT_NEAR(hit.normal.x(), -1.0, EPS);
}

TEST(VoxelVolumeTest, VolumeBelowThresholdIsMissed) {
    // Arrange
    VoxelVolume volume;
    volume.init(4, 4, 4);
    volume.putVoxel(1, 1, 1, (char)100);
    Ray ray(Vector3Dd(-0.25, -0.25, 5), Vector3Dd(0, 0, -1));
    RayHit first;
    RayHit second;

    // Act & Assert
    EXPECT_FALSE(volume.doIntersectionFirstHit(ray, &first));
    volume.setThreshold(50);
    EXPECT_TRUE(volume.doIntersectionFirstHit(ray, &second));
}

TEST(VoxelVolumeTest, FirstHitOfRandomRaysMatchesTheNearestFilledVoxel) {
    // Arrange
    java::Random random(1234);
    int n = 7;
    VoxelVolume volume;
    volume.init(n, n, n);
    for ( int z = 0; z < n; z++ ) {
        for ( int y = 0; y < n; y++ ) {
            for ( int x = 0; x < n; x++ ) {
                if ( random.nextDouble() < 0.08 ) {
                    volume.putVoxel(x, y, z, FILLED);
                }
            }
        }
    }

    for ( int i = 0; i < 2000; i++ ) {
        // Rays from outside the volume cube, aimed at a point inside it
        Vector3Dd origin = randomDirection(random).multiply(4.0);
        Vector3Dd target(random.nextDouble() * 2 - 1,
            random.nextDouble() * 2 - 1, random.nextDouble() * 2 - 1);
        Vector3Dd direction = target.subtract(origin).normalized();
        Ray ray(origin, direction);
        RayHit hit(RayHit::DETAIL_NONE, true);

        // Act
        bool intersects = volume.doIntersectionFirstHit(ray, &hit);
        double expected = bruteForceFirstHit(volume, origin, direction);

        // Assert
        if ( std::isinf(expected) ) {
            EXPECT_FALSE(intersects) << "ray " << i;
        }
        else {
            ASSERT_TRUE(intersects) << "ray " << i;
            EXPECT_NEAR(hit.getRay()->getT(), expected, 1.0e-7) << "ray " << i;
        }
    }
}

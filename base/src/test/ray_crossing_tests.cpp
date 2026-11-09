#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include <gtest/gtest.h>
#include <vector>
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"

namespace {
class Crossings : public RayCrossingSink {
public:
    std::vector<double> roots;
    std::vector<RayCrossing> events;
    void crossing(const RayCrossing& event) override {
        roots.push_back(event.t);
        events.push_back(event);
    }
};
}

TEST(RayCrossings, SpherePreservesAffineParameterAndOrdersRoots) {
    Sphere sphere(1);
    Ray ray;
    ray.setOriginAndDirection(Vector3Dd(-3,0,0), Vector3Dd(2,0,0));
    Crossings hits;
    EXPECT_EQ(RayCrossingSupport::Supported,
        sphere.emitRayCrossings(ray, RayCrossingQuery(0,10), hits));
    ASSERT_EQ(2u, hits.roots.size());
    EXPECT_DOUBLE_EQ(1, hits.roots[0]);
    EXPECT_DOUBLE_EQ(2, hits.roots[1]);
    EXPECT_EQ(&sphere, hits.events[0].geometry);
    EXPECT_EQ(RayCrossing::Kind::Entering, hits.events[0].kind);
    EXPECT_EQ(RayCrossing::Kind::Leaving, hits.events[1].kind);
}

TEST(RayCrossings, InteriorTangentAndClippedIntervals) {
    Sphere sphere(1);
    Crossings inside, tangent, clipped, zero;
    sphere.emitRayCrossings(Ray(Vector3Dd(0,0,0),Vector3Dd(1,0,0)),
        RayCrossingQuery(0,10), inside);
    ASSERT_EQ(1u, inside.roots.size());
    EXPECT_DOUBLE_EQ(1, inside.roots[0]);
    sphere.emitRayCrossings(Ray(Vector3Dd(-2,1,0),Vector3Dd(1,0,0)),
        RayCrossingQuery(0,10), tangent);
    ASSERT_EQ(1u, tangent.roots.size());
    EXPECT_DOUBLE_EQ(2, tangent.roots[0]);
    EXPECT_EQ(RayCrossing::Kind::Tangent, tangent.events[0].kind);
    sphere.emitRayCrossings(Ray(Vector3Dd(-3,0,0),Vector3Dd(1,0,0)),
        RayCrossingQuery(2.5,3.5), clipped);
    EXPECT_TRUE(clipped.roots.empty());
    Ray invalid;
    invalid.setDirection(Vector3Dd(0,0,0));
    sphere.emitRayCrossings(invalid, RayCrossingQuery(0,10), zero);
    EXPECT_TRUE(zero.roots.empty());
}

TEST(RayCrossings, UnsupportedIsDifferentFromMiss) {
    Box box(1,1,1);
    Sphere sphere(1);
    Ray ray(Vector3Dd(3,0,0), Vector3Dd(1,0,0));
    Crossings hits;
    EXPECT_EQ(RayCrossingSupport::Unsupported,
        box.emitRayCrossings(ray, RayCrossingQuery(0,10), hits));
    EXPECT_EQ(RayCrossingSupport::Supported,
        sphere.emitRayCrossings(ray, RayCrossingQuery(0,10), hits));
    EXPECT_TRUE(hits.roots.empty());
}

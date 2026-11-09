#include <gtest/gtest.h>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/Entity.h"
#include "vsdk/toolkit/common/EntityListener.h"
#include "vsdk/toolkit/environment/geometry/volume/Cone.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"

namespace {

class RecordingListener : public EntityListener {
public:
    int updated = 0;
    int deleted = 0;
    Entity *lastSource = nullptr;

    virtual void notifyEntityEvent(const EntityEvent &event) override {
        lastSource = event.getSource();
        if ( event.getType() == EntityEvent::UPDATED ) {
            updated++;
        }
        else {
            deleted++;
        }
    }
};

}

TEST(EntityTest, GeometryConstructorsDeclareControlSpecifications) {
    Sphere sphere(1.0);
    ASSERT_EQ(1, sphere.getControlSpecifications().size());
    EXPECT_TRUE(sphere.getControlSpecifications().get(0) ==
        "double;radius;(0, INFINITE)");

    Cone cone(1.0, 0.0, 2.0);
    ASSERT_EQ(3, cone.getControlSpecifications().size());
    EXPECT_TRUE(cone.getControlSpecifications().get(0) ==
        "double;bottomRadius;(0, INFINITE)");
    EXPECT_TRUE(cone.getControlSpecifications().get(1) ==
        "double;topRadius;[0, INFINITE)");
    EXPECT_TRUE(cone.getControlSpecifications().get(2) ==
        "double;height;(0, INFINITE)");

    Entity entity;
    EXPECT_EQ(0, entity.getControlSpecifications().size());
}

TEST(EntityTest, SemanticAccessorsForSphereAndCone) {
    Sphere sphere(1.0);
    sphere.setRadius(2.5);
    EXPECT_DOUBLE_EQ(2.5, sphere.getRadius());
    EXPECT_DOUBLE_EQ(6.25, sphere.getRadiusSquared());

    Cone cone(1.0, 0.5, 2.0);
    cone.setBottomRadius(3.0);
    cone.setTopRadius(0.0);
    cone.setHeight(4.0);
    EXPECT_DOUBLE_EQ(3.0, cone.getBottomRadius());
    EXPECT_DOUBLE_EQ(0.0, cone.getTopRadius());
    EXPECT_DOUBLE_EQ(4.0, cone.getHeight());
}

TEST(EntityTest, UpdateAndDisposeNotifySubscribers) {
    RecordingListener listener;
    Sphere sphere(1.0);
    sphere.addEntityListener(&listener);
    sphere.addEntityListener(&listener);

    sphere.update();
    EXPECT_EQ(1, listener.updated);
    EXPECT_EQ(&sphere, listener.lastSource);

    sphere.dispose();
    sphere.update();
    EXPECT_EQ(1, listener.deleted);
    EXPECT_EQ(1, listener.updated);
}

TEST(EntityTest, DestructorEmitsDeleted) {
    RecordingListener listener;
    {
        Cone cone(1.0, 0.0, 2.0);
        cone.addEntityListener(&listener);
    }
    EXPECT_EQ(1, listener.deleted);
}

TEST(EntityTest, CopiesKeepSpecificationsButNotSubscribers) {
    RecordingListener listener;
    Sphere original(1.0);
    original.addEntityListener(&listener);

    Sphere copy(original);
    copy.update();
    EXPECT_EQ(0, listener.updated);
    EXPECT_EQ(1, copy.getControlSpecifications().size());

    original.removeEntityListener(&listener);
    original.update();
    EXPECT_EQ(0, listener.updated);
}

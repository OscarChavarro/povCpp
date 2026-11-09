#include "java/lang/String.h"
#include <gtest/gtest.h>


#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/Entity.h"
#include "vsdk/toolkit/environment/geometry/volume/Cone.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/gui/editor/ControlSpecification.h"
#include "vsdk/toolkit/gui/editor/GenericEditor.h"
#include "vsdk/toolkit/gui/editor/GenericEditorListener.h"

namespace {

/** Records the presentation hooks instead of creating widgets. */
class RecordingEditor : public GenericEditor {
public:
    java::ArrayList<java::String> controls;
    bool hasMessage = false;
    java::String message;
    java::String clearedMessage;

    const ControlSpecification *at(long int i) const
    {
        return specifications.get(i);
    }

protected:
    void beginBuild(const java::String &) override
    {
        controls.clear();
    }

    void addControl(const ControlSpecification *specification,
                    const java::String &value) override
    {
        controls.add(java::String(specification->getLabel().c_str()) +
            "=" + value.c_str());
    }

    void endBuild() override {}

    void showValidationMessage(const char *text) override
    {
        hasMessage = text != nullptr;
        message = text != nullptr ? text : "";
    }

    void setControlValue(const ControlSpecification *specification,
                         const java::String &value) override
    {
        java::String prefix =
            java::String(specification->getLabel().c_str()) + "=";
        for ( size_t i = 0; i < controls.size(); i++ ) {
            if ( controls[i].startsWith(prefix.c_str()) ) {
                controls[i] = prefix + value.c_str();
            }
        }
    }

    void clearControls(const java::String &text) override
    {
        controls.clear();
        clearedMessage = text.c_str();
    }
};

class CountingListener : public GenericEditorListener {
public:
    java::ArrayList<Entity *> changed;
    void notifyEntityChanged(Entity *entity) override
    {
        changed.add(entity);
    }
};

/** Entity with accessors of every supported type, one of them non-const */
class Counters : public Entity {
private:
    int count;
    long long total;
    float ratio;
    double weight;

public:
    Counters() : count(1), total(10), ratio(0.1f), weight(0.0)
    {
        addControlSpecification("int;count;[1, 10]",
            &Counters::getCount, &Counters::setCount);
        addControlSpecification("long;total",
            &Counters::getTotal, &Counters::setTotal);
        addControlSpecification("float;ratio;[0, 1]",
            &Counters::getRatio, &Counters::setRatio);
        // Type mismatch with the accessors: skipped by the editor
        addControlSpecification("int;weight",
            &Counters::getWeight, &Counters::setWeight);
        // No accessors registered: skipped by the editor
        getControlSpecifications().add("double;missing");
    }

    int getCount() const { return count; }
    void setCount(int value) { count = value; }
    long long getTotal() { return total; }
    void setTotal(long long value) { total = value; }
    float getRatio() const { return ratio; }
    void setRatio(float value) { ratio = value; }
    double getWeight() const { return weight; }
    void setWeight(double value) { weight = value; }
};

template <typename... Items>
java::ArrayList<java::String> list(Items... items)
{
    const char *values[] = { items... };
    java::ArrayList<java::String> result;
    for ( const char *value : values ) {
        result.add(value);
    }
    return result;
}

}

TEST(GenericEditorTest, ParsesOpenAndClosedIntervals) {
    ControlSpecification *open = ControlSpecification::parse(
        "double;radius;(0, INFINITE)");
    ASSERT_NE(nullptr, open);
    EXPECT_TRUE(open->getType() == "double");
    EXPECT_TRUE(open->getName() == "radius");
    EXPECT_TRUE(open->getLabel() == "Radius");
    EXPECT_FALSE(open->contains(0.0));
    EXPECT_FALSE(open->contains(-1.0));
    EXPECT_TRUE(open->contains(1e-9));
    EXPECT_TRUE(open->contains(1e300));
    delete open;

    ControlSpecification *closed = ControlSpecification::parse(
        "int;count;[1, 10]");
    ASSERT_NE(nullptr, closed);
    EXPECT_TRUE(closed->contains(1));
    EXPECT_TRUE(closed->contains(10));
    EXPECT_FALSE(closed->contains(0));
    EXPECT_FALSE(closed->contains(11));
    delete closed;

    ControlSpecification *unbounded = ControlSpecification::parse(
        "double;offset");
    ASSERT_NE(nullptr, unbounded);
    EXPECT_TRUE(unbounded->contains(-1e300));
    delete unbounded;
}

TEST(GenericEditorTest, RejectsMalformedSpecifications) {
    EXPECT_EQ(nullptr, ControlSpecification::parse("double"));
    EXPECT_EQ(nullptr, ControlSpecification::parse("double;radius;0, 1"));
    EXPECT_EQ(nullptr, ControlSpecification::parse("double;radius;(1, 0)"));
    EXPECT_EQ(nullptr, ControlSpecification::parse("double;radius;(a, 1)"));
}

TEST(GenericEditorTest, EditsSphereRadiusThroughAccessors) {
    Sphere sphere(1.0);
    RecordingEditor editor;
    CountingListener listener;
    editor.setListener(&listener);

    editor.build(&sphere);
    EXPECT_TRUE(list("Radius=1.0").equals(editor.controls));

    EXPECT_TRUE(editor.updateValue(editor.at(0), "2.5"));
    EXPECT_DOUBLE_EQ(2.5, sphere.getRadius());
    EXPECT_DOUBLE_EQ(6.25, sphere.getRadiusSquared());
    EXPECT_EQ(1u, listener.changed.size());
    EXPECT_FALSE(editor.hasMessage);

    EXPECT_FALSE(editor.updateValue(editor.at(0), "0"));
    EXPECT_FALSE(editor.updateValue(editor.at(0), "-3"));
    EXPECT_FALSE(editor.updateValue(editor.at(0), "abc"));
    EXPECT_DOUBLE_EQ(2.5, sphere.getRadius());
    EXPECT_EQ(1u, listener.changed.size());
    EXPECT_TRUE(editor.hasMessage);
}

TEST(GenericEditorTest, EditsConeAttributesThroughAccessors) {
    Cone cone(1.0, 0.0, 2.0);
    RecordingEditor editor;

    editor.build(&cone);
    EXPECT_TRUE(list("BottomRadius=1.0", "TopRadius=0.0", "Height=2.0").equals(editor.controls));

    const ControlSpecification *bottom = editor.at(0);
    const ControlSpecification *top = editor.at(1);
    const ControlSpecification *height = editor.at(2);
    EXPECT_FALSE(editor.updateValue(bottom, "0"));
    EXPECT_TRUE(editor.updateValue(top, "0"));
    EXPECT_TRUE(editor.updateValue(top, "0.5"));
    EXPECT_FALSE(editor.updateValue(height, "0"));
    EXPECT_TRUE(editor.updateValue(height, "3"));
    EXPECT_DOUBLE_EQ(1.0, cone.getBottomRadius());
    EXPECT_DOUBLE_EQ(0.5, cone.getTopRadius());
    EXPECT_DOUBLE_EQ(3.0, cone.getHeight());
}

TEST(GenericEditorTest, EditsEveryAccessorType) {
    Counters counters;
    RecordingEditor editor;

    editor.build(&counters);
    EXPECT_TRUE(list("Count=1", "Total=10", "Ratio=0.1").equals(editor.controls));

    EXPECT_TRUE(editor.updateValue(editor.at(0), " 7 "));
    EXPECT_FALSE(editor.updateValue(editor.at(0), "11"));
    EXPECT_FALSE(editor.updateValue(editor.at(0), "2.5"));
    EXPECT_TRUE(editor.updateValue(editor.at(1), "-9000000000"));
    EXPECT_TRUE(editor.updateValue(editor.at(2), "0.25"));
    EXPECT_FALSE(editor.updateValue(editor.at(2), "2"));
    EXPECT_EQ(7, counters.getCount());
    EXPECT_EQ(-9000000000LL, counters.getTotal());
    EXPECT_FLOAT_EQ(0.25f, counters.getRatio());
    EXPECT_TRUE(list("Count=7", "Total=-9000000000", "Ratio=0.25").equals(editor.controls));
}

TEST(GenericEditorTest, CopiedEntitiesKeepTheirAccessors) {
    Sphere original(1.0);
    Sphere copy(original);
    copy.setRadius(3.0);
    RecordingEditor editor;

    editor.build(&copy);
    EXPECT_TRUE(list("Radius=3.0").equals(editor.controls));
    EXPECT_TRUE(editor.updateValue(editor.at(0), "4"));
    EXPECT_DOUBLE_EQ(4.0, copy.getRadius());
    EXPECT_DOUBLE_EQ(1.0, original.getRadius());
}

TEST(GenericEditorTest, RefreshesControlsWhenEntityIsUpdated) {
    Sphere sphere(1.0);
    RecordingEditor editor;
    editor.build(&sphere);

    // A change made by another tool, announced with update()
    sphere.setRadius(4.0);
    sphere.update();
    EXPECT_TRUE(list("Radius=4.0").equals(editor.controls));
}

TEST(GenericEditorTest, ClearsControlsWhenEntityIsDisposed) {
    Sphere sphere(1.0);
    RecordingEditor editor;
    editor.build(&sphere);

    sphere.dispose();
    EXPECT_TRUE(editor.isEntityDeleted());
    EXPECT_EQ(nullptr, editor.getEntity());
    EXPECT_TRUE(editor.controls.isEmpty());
    EXPECT_STREQ("Entity deleted (Sphere).", editor.clearedMessage.c_str());
    ControlSpecification *radius = ControlSpecification::parse(
        "double;radius;(0, INFINITE)");
    EXPECT_FALSE(editor.updateValue(radius, "2"));
    delete radius;
}

TEST(GenericEditorTest, ClearsControlsWhenEntityIsDestroyed) {
    RecordingEditor editor;
    {
        Cone cone(1.0, 0.0, 2.0);
        editor.build(&cone);
    }
    EXPECT_TRUE(editor.isEntityDeleted());
    EXPECT_STREQ("Entity deleted (Cone).", editor.clearedMessage.c_str());
}

TEST(GenericEditorTest, StopsListeningToPreviousEntity) {
    Sphere first(1.0);
    Sphere second(2.0);
    RecordingEditor editor;
    editor.build(&first);
    editor.build(&second);

    first.dispose();
    EXPECT_FALSE(editor.isEntityDeleted());
    EXPECT_TRUE(list("Radius=2.0").equals(editor.controls));
}

TEST(GenericEditorTest, ReportsEntitiesWithoutSpecifications) {
    RecordingEditor editor;
    Entity entity;
    editor.build(&entity);
    EXPECT_TRUE(editor.controls.isEmpty());
    EXPECT_TRUE(editor.hasMessage);
}

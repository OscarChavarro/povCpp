#ifndef __WIDGET_VARIABLE__
#define __WIDGET_VARIABLE__

#include "vsdk/toolkit/gui/widget/WidgetElement.h"

/**
A WidgetVariable is a value stored at computer memory which has a type, and is
assigned to a name and that is inside a valid values range. For example, the
radius of an sphere has a valid value range expressed as an interval: "[0,
INF]". A current value for that variable could be the number "5.0", and its
name could be "r". This variable is of type "WidgetDoubleVariable".

This class is the superclass of several other classes, each one representing
an specific variable type.

This class and its subclasses plays a client role in a reflection design
pattern.

This class plays a role of leaf on an n-ary tree in the composite design
pattern.
*/
class WidgetVariable : public WidgetElement {
protected:
    /// Variable names follows a convention of scope operator. Example:
    /// "position" is a global name, "camera.position" is the same variable
    /// under the "camera" scope. "scene.camera.position" could be a full
    /// hierarchy name for a variable inside the system.
    java::String name;
    java::String validRange;
    java::String initialvalue;

public:
    WidgetVariable() : name(""), validRange(""), initialvalue("") {}
    virtual ~WidgetVariable() {}

    const java::String& getName() const { return name; }
    void setName(const java::String& name) { this->name = name; }

    const java::String& getInitialvalue() const { return initialvalue; }
    void setInitialvalue(const java::String& initialvalue)
    {
        this->initialvalue = initialvalue;
    }

    virtual java::String toString() const override
    {
        java::String msg = "";
        msg = msg + "VARIABLE:\n"
            + "     TYPE: " + getType() + "\n"
            + "     NAME: " + getName() + "\n"
            + "     INITIAL_VALUE: " + getInitialvalue() + "\n"
            + "     VALID_RANGE: " + getValidRange() + "\n";
        return msg;
    }

    /**
    Each variable has a type. Examples: "Integer", "Double", "String",
    "Vector3Dd".
    @return variable type name
    */
    virtual java::String getType() const = 0;

    /**
    Gets the current String specifying valid value range. The returned String
    contains an specification expressed in Vitral GUI value ranges language.
    @return valid value range specification
    */
    virtual java::String getValidRange() const = 0;

    /**
    Sets the current String specifying valid value range. The returned String
    contains an specification expressed in Vitral GUI value ranges language.
    @param vr valid value range specification
    */
    virtual void setValidRange(const java::String& vr) = 0;
};

#endif

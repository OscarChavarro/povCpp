#ifndef __WIDGET_ELEMENT__
#define __WIDGET_ELEMENT__

#include "java/lang/String.h"
#include "vsdk/toolkit/gui/PresentationElement.h"

class Widget;

class WidgetElement : public PresentationElement {
protected:
    Widget* context;

public:
    WidgetElement() : context(nullptr) {}
    virtual ~WidgetElement() {}

    virtual java::String toString() const = 0;
};

#endif

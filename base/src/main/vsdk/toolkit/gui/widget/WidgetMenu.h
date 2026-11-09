#ifndef __WIDGET_MENU__
#define __WIDGET_MENU__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/gui/widget/WidgetMenuElement.h"

/**
Menu (internal node of a menu tree). Children are owned by the `Widget`
context, not by the menu, as a submenu can also be registered as a popup.
*/
class WidgetMenu : public WidgetMenuElement {
private:
    java::ArrayList<WidgetMenuElement*> children;
    java::String name;
    char mnemonic;
    java::String accelerator;

public:
    explicit WidgetMenu(Widget* c);
    virtual ~WidgetMenu() {}

    java::ArrayList<WidgetMenuElement*>& getChildren();
    void setName(const java::String& n);
    void addChild(WidgetMenuElement* i);
    virtual java::String toString(int level) const override;
    virtual java::String toString() const override;
    virtual bool isMenu() const override { return true; }
    const java::String& getName() const;
    char getMnemonic() const;
    const java::String& getAccelerator() const;
};

#endif

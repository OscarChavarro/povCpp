#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/gui/widget/WidgetMenu.h"

WidgetMenu::WidgetMenu(Widget* c) : name(""), mnemonic('\0'), accelerator("")
{
    context = c;
}

java::ArrayList<WidgetMenuElement*>& WidgetMenu::getChildren()
{
    return children;
}

void WidgetMenu::setName(const java::String& n)
{
    name = processSimplifiedName(n);
    mnemonic = processMnemonic(n);
    accelerator = processAccelerator(n);
}

void WidgetMenu::addChild(WidgetMenuElement* i)
{
    children.add(i);
}

java::String WidgetMenu::toString(int level) const
{
    java::String leadingSpace = "";
    int j;

    for ( j = 0; j < level; j++ ) {
        leadingSpace = leadingSpace + "  ";
    }

    java::String msg = leadingSpace + "Menu \"" + name + "\"\n";

    int i;

    for ( i = 0; i < children.size(); i++ ) {
        msg = msg + (children.get(i))->toString(level+1);
    }

    return msg;
}

java::String WidgetMenu::toString() const
{
    return toString(0);
}

const java::String& WidgetMenu::getName() const
{
    return name;
}

char WidgetMenu::getMnemonic() const
{
    return mnemonic;
}

const java::String& WidgetMenu::getAccelerator() const
{
    return accelerator;
}

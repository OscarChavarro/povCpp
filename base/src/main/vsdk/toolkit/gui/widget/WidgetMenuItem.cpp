#include "vsdk/toolkit/gui/widget/WidgetMenuItem.h"

WidgetMenuItem::WidgetMenuItem(Widget* c)
    : name(""), hasName(false), commandName(""), hasCommandName(false),
      isSeparatorFlag(false), isGrayedFlag(false), isCheckableFlag(false),
      isCheckedFlag(false), mnemonic(0), accelerator("")
{
    context = c;
}

bool WidgetMenuItem::isSeparator() const
{
    return isSeparatorFlag;
}

bool WidgetMenuItem::isGrayed() const
{
    return isGrayedFlag;
}

bool WidgetMenuItem::isCheckable() const
{
    return isCheckableFlag;
}

bool WidgetMenuItem::isChecked() const
{
    return isCheckedFlag;
}

void WidgetMenuItem::setName(const java::String& n)
{
    name = processSimplifiedName(n);
    hasName = true;
    mnemonic = processMnemonic(n);
    accelerator = processAccelerator(n);
}

java::String WidgetMenuItem::getName() const
{
    if ( !hasName ) return "No Name";
    return name;
}

java::String WidgetMenuItem::getCommandName() const
{
    if ( !hasCommandName ) return "IDC_NO_COMMAND";
    return commandName;
}

void WidgetMenuItem::setCommandName(const java::String& a)
{
    commandName = a;
    hasCommandName = true;
}

void WidgetMenuItem::addModifier(const java::String& m)
{
    if ( m.equals("CHECKED") ) {
        isCheckableFlag = true;
        isCheckedFlag = true;
    }
    else if ( m.equals("GRAYED") ) {
        isGrayedFlag = true;
    }
    else if ( m.equals("UNCHEKED") ) {
        isCheckableFlag = true;
    }
    else if ( m.equals("SEPARATOR") ) {
        isSeparatorFlag = true;
    }
    else {
        setCommandName(m);
    }
}

java::String WidgetMenuItem::toString(int level) const
{
    java::String leadingSpace = "";
    int j;

    for ( j = 0; j < level; j++ ) {
        leadingSpace = leadingSpace + "  ";
    }

    java::String msg = leadingSpace + " - MenuItem ";

    if ( isSeparatorFlag ) {
        msg = msg + "--- SEPARATOR ---";
    }
    else {
        msg = msg + "\"" + name + "\"";
    }

    msg = msg + "\n";

    return msg;
}

java::String WidgetMenuItem::toString() const
{
    return toString(0);
}

char WidgetMenuItem::getMnemonic() const
{
    return mnemonic;
}

const java::String& WidgetMenuItem::getAccelerator() const
{
    return accelerator;
}

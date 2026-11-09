#ifndef __WIDGET_MENU_ELEMENT__
#define __WIDGET_MENU_ELEMENT__

#include "vsdk/toolkit/gui/widget/WidgetElement.h"

class WidgetMenuElement : public WidgetElement {
private:
    static int fromHex(char c);

protected:
    /**
    Given a Windows32 SDK API / Aquynza style coded name, this method
    generates the simplified name, separating from it its mnemonic and
    accelerator if any.

    For example, if codedName has the value <code>&Open\tCtrl+O</code>, the
    simplified name will be "Open", the mnemonic will be 'O' and the
    accelerator will be "Ctrl+O". This method return its simplified name.
    Unicode escape sequences (`#XXXX`) are converted to UTF-8.
    */
    static java::String processSimplifiedName(const java::String& codedName);

    /**
    Same as `processSimplifiedName`, but returns the mnemonic, or '\0'.
    */
    static char processMnemonic(const java::String& codedName);

    /**
    Same as `processSimplifiedName`, but returns the accelerator, or an empty
    string if there is no accelerator.
    */
    static java::String processAccelerator(const java::String& codedName);

public:
    virtual ~WidgetMenuElement() {}

    using WidgetElement::toString;
    virtual java::String toString(int level) const = 0;

    /**
    @return true for a `WidgetMenu`, false for a `WidgetMenuItem`
    */
    virtual bool isMenu() const { return false; }
};

#endif

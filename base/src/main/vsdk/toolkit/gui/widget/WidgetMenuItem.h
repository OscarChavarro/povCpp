#ifndef __WIDGET_MENU_ITEM__
#define __WIDGET_MENU_ITEM__

#include "vsdk/toolkit/gui/widget/WidgetMenuElement.h"

class WidgetMenuItem : public WidgetMenuElement {
private:
    java::String name;
    bool hasName;
    java::String commandName;
    bool hasCommandName;
    bool isSeparatorFlag;
    bool isGrayedFlag;
    bool isCheckableFlag;
    bool isCheckedFlag;
    char mnemonic;
    java::String accelerator;

public:
    explicit WidgetMenuItem(Widget* c);
    virtual ~WidgetMenuItem() {}

    bool isSeparator() const;
    bool isGrayed() const;
    bool isCheckable() const;
    bool isChecked() const;
    void setName(const java::String& n);
    java::String getName() const;
    java::String getCommandName() const;
    void setCommandName(const java::String& a);
    void addModifier(const java::String& m);
    virtual java::String toString(int level) const override;
    virtual java::String toString() const override;
    char getMnemonic() const;
    const java::String& getAccelerator() const;
};

#endif

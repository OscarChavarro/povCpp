#ifndef __WIDGET_DIALOG__
#define __WIDGET_DIALOG__

#include "java/lang/String.h"

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/gui/widget/WidgetDialogElement.h"
#include "vsdk/toolkit/gui/widget/WidgetElement.h"

/**
This class plays a role of internal node on an n-ary tree in the composite
design pattern. Children are owned by the `Widget` context.
*/
class WidgetDialog : public WidgetElement {
private:
    java::String id;
    java::String name;
    int orientation;
    java::ArrayList<WidgetElement*> widgetElementList;
    /// In the importing from file process, a dialog could be "incomplete",
    /// due to having a variable reference to a variable that is not loaded
    /// yet.  In such situation, a two pass processing is performed:
    /// On first pass does not create neither associate any variable, but store
    /// its incomplete references (names) on this ArrayList. On second pass
    /// the factory traverse this list in order to add current variables
    /// from context. Check WidgetPersistence.importAquynzaWidgetDialog method.
    java::ArrayList<java::String> pendingVariableNames;
    java::ArrayList<java::String> pendingCommandNames;
    java::ArrayList<java::String> pendingDialogNames;
    java::ArrayList<java::String> pendingDialogRefNames;
    bool collapsable;
    java::String titleKey;
    bool modal;
    java::ArrayList<WidgetDialogElement> elements;

public:
    static const int ORIENTATION_HORIZONTAL = 0x01;
    static const int ORIENTATION_VERTICAL = 0x02;

    WidgetDialog();
    virtual ~WidgetDialog() {}

    /**
    @return key of the message of the title of the dialog
    */
    const java::String& getTitleKey() const;
    void setTitleKey(const java::String& titleKey);
    bool isModal() const;
    void setModal(bool modal);

    /**
    @return the ordered, toolkit-independent content of the dialog
    */
    const java::ArrayList<WidgetDialogElement>& getElements() const;
    void addElement(const WidgetDialogElement& element);

    /**
    @return the first element (in depth) with the given action, or null
    */
    const WidgetDialogElement* getElementByAction(
        const java::String& action) const;

    bool isCollapsable() const;
    void setCollapsable(bool collapsable);
    java::ArrayList<java::String>& getPendingCommandNames();
    void setPendingCommandNames(
        const java::ArrayList<java::String>& pendingCommandNames);
    java::ArrayList<WidgetElement*>& getWidgetElementList();
    void setWidgetElementList(
        const java::ArrayList<WidgetElement*>& widgetElementList);
    const java::String& getId() const;
    void setId(const java::String& id);
    const java::String& getName() const;
    void setName(const java::String& name);
    double getOrientation() const;
    void setOrientation(int orientation);
    java::ArrayList<java::String>& getPendingVariableNames();
    void setPendingVariableNames(
        const java::ArrayList<java::String>& pendingVariableNames);
    java::ArrayList<java::String>& getPendingDialogNames();
    void setPendingDialogNames(
        const java::ArrayList<java::String>& pendingDialogNames);
    java::ArrayList<WidgetElement*>& getChildren();
    void setChildren(const java::ArrayList<WidgetElement*>& widgetElementList);
    java::ArrayList<java::String>& getPendingDialogRefNames();
    void setPendingDialogRefNames(
        const java::ArrayList<java::String>& pendingDialogRefNames);

    /**
    Given a variableName, this method asks the context for a WidgetVariable
    pointer in order to reference the given variable. If that variable
    doesn't exist, an error is reported.
    */
    void associateVariable(const java::String& variableName);

    virtual java::String toString() const override;
};

#endif

#ifndef __WIDGET__
#define __WIDGET__

#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "java/util/HashMap.h"
#include "vsdk/toolkit/gui/PresentationElement.h"

class WidgetButtonGroup;
class WidgetCommand;
class WidgetDialog;
class WidgetElement;
class WidgetMenu;
class WidgetVariable;

/**
In order to understand this class, the following concepts must be taken into
account: - WidgetVariable - WidgetCommand - Reflection (introspection) design
pattern - Menubars, buttons bars, and other are based upon WidgetCommands -
Dialogs are based upon WidgetVariables and WidgetCommands - Dialogs and menus are
hierarchical

C++ port note: a Widget owns every element reachable from it (commands,
variables, button groups, dialogs and their children, menubar, popups and
their children); shared elements are deleted once.
*/
class Widget : public PresentationElement {
private:
    // Basic / fundamental / atomic elements
    java::ArrayList<WidgetCommand*> commandList;
    java::ArrayList<WidgetVariable*> variableList;
    java::HashMap<java::String, java::String> messagesTable;
    // Composite elements
    WidgetMenu* menubar;
    java::ArrayList<WidgetMenu*> popupMenuList;
    java::ArrayList<WidgetButtonGroup*> buttonGroupList;
    java::ArrayList<WidgetDialog*> dialogList;

    Widget(const Widget& other);
    Widget& operator=(const Widget& other);

public:
    Widget();
    virtual ~Widget();

    java::ArrayList<WidgetDialog*>& getDialogList();
    void setDialogList(const java::ArrayList<WidgetDialog*>& dialogList);
    void addMessage(const java::String& id, const java::String& message);

    /**
    @param id message identifier
    @return registered message, or the id itself if it has no message
    */
    java::String getMessage(const java::String& id) const;
    void setMenubar(WidgetMenu* m);
    WidgetMenu* getMenubar() const;
    WidgetCommand* getCommandByName(const java::String& name) const;
    WidgetButtonGroup* getButtonGroup(const java::String& name) const;
    WidgetMenu* getPopup(const java::String& name) const;
    void addPopupMenu(WidgetMenu* p);
    void addCommand(WidgetCommand* c);
    void addButtonGroup(WidgetButtonGroup* b);
    void addDialog(WidgetDialog* dialog);

    /**
    @return the dialog with the given identifier, or null
    */
    WidgetDialog* getDialogById(const java::String& id) const;
    void addVariable(WidgetVariable* variable);
    WidgetVariable* getVariableByName(const java::String& name) const;
    java::String toString() const;
};

#endif

#include "java/util/ArrayList.txx"
#include "java/util/HashMap.h"
#include "vsdk/toolkit/gui/widget/Widget.h"
#include "vsdk/toolkit/gui/widget/WidgetButtonGroup.h"
#include "vsdk/toolkit/gui/widget/WidgetCommand.h"
#include "vsdk/toolkit/gui/widget/WidgetDialog.h"
#include "vsdk/toolkit/gui/widget/WidgetMenu.h"
#include "vsdk/toolkit/gui/widget/variable/WidgetVariable.h"

namespace {

bool containsElement(const java::ArrayList<WidgetElement*>& list,
                     const WidgetElement* element)
{
    long i;
    for ( i = 0; i < list.size(); i++ ) {
        if ( list.get(i) == element ) {
            return true;
        }
    }
    return false;
}

void collect(java::ArrayList<WidgetElement*>& owned, WidgetElement* element)
{
    if ( element == nullptr || containsElement(owned, element) ) {
        return;
    }
    owned.add(element);

    WidgetMenu* menu = dynamic_cast<WidgetMenu*>(element);
    if ( menu != nullptr ) {
        long i;
        for ( i = 0; i < menu->getChildren().size(); i++ ) {
            collect(owned, menu->getChildren().get(i));
        }
    }
    WidgetDialog* dialog = dynamic_cast<WidgetDialog*>(element);
    if ( dialog != nullptr ) {
        long i;
        for ( i = 0; i < dialog->getChildren().size(); i++ ) {
            collect(owned, dialog->getChildren().get(i));
        }
    }
}

}

Widget::Widget() : menubar(nullptr)
{
}

Widget::~Widget()
{
    java::ArrayList<WidgetElement*> owned;
    long i;

    for ( i = 0; i < commandList.size(); i++ ) {
        collect(owned, commandList.get(i));
    }
    for ( i = 0; i < variableList.size(); i++ ) {
        collect(owned, variableList.get(i));
    }
    collect(owned, menubar);
    for ( i = 0; i < popupMenuList.size(); i++ ) {
        collect(owned, popupMenuList.get(i));
    }
    for ( i = 0; i < buttonGroupList.size(); i++ ) {
        collect(owned, buttonGroupList.get(i));
    }
    for ( i = 0; i < dialogList.size(); i++ ) {
        collect(owned, dialogList.get(i));
    }
    for ( i = 0; i < owned.size(); i++ ) {
        delete owned.get(i);
    }
}

java::ArrayList<WidgetDialog*>& Widget::getDialogList()
{
    return dialogList;
}

void Widget::setDialogList(const java::ArrayList<WidgetDialog*>& dialogList)
{
    this->dialogList = dialogList;
}

void Widget::addMessage(const java::String& id, const java::String& message)
{
    messagesTable.put(id, message);
}

java::String Widget::getMessage(const java::String& id) const
{
    const java::String* msg;

    msg = messagesTable.get(id);

    if ( msg == nullptr ) {
        return id;
    }
    return *msg;
}

void Widget::setMenubar(WidgetMenu* m)
{
    menubar = m;
}

WidgetMenu* Widget::getMenubar() const
{
    return menubar;
}

WidgetCommand* Widget::getCommandByName(const java::String& name) const
{
    WidgetCommand* command = nullptr;
    WidgetCommand* candidate;
    int i;

    for ( i = 0; i < commandList.size(); i++ ) {
        candidate = commandList.get(i);
        if ( candidate->getId().equals(name) ) {
            command = candidate;
            break;
        }
    }
    return command;
}

WidgetButtonGroup* Widget::getButtonGroup(const java::String& name) const
{
    WidgetButtonGroup* group = nullptr;
    WidgetButtonGroup* candidate;
    int i;

    for ( i = 0; i < buttonGroupList.size(); i++ ) {
        candidate = buttonGroupList.get(i);
        if ( candidate->getName().equals(name) ) {
            group = candidate;
            break;
        }
    }
    return group;
}

WidgetMenu* Widget::getPopup(const java::String& name) const
{
    WidgetMenu* menu = nullptr;
    WidgetMenu* candidate;

    int i;
    for ( i = 0; i < popupMenuList.size(); i++ ) {
        candidate = popupMenuList.get(i);
        if ( candidate->getName().equals(name) ) {
            menu = candidate;
            break;
        }
    }
    return menu;
}

void Widget::addPopupMenu(WidgetMenu* p)
{
    popupMenuList.add(p);
}

void Widget::addCommand(WidgetCommand* c)
{
    commandList.add(c);
}

void Widget::addButtonGroup(WidgetButtonGroup* b)
{
    buttonGroupList.add(b);
}

void Widget::addDialog(WidgetDialog* dialog)
{
    dialogList.add(dialog);
}

WidgetDialog* Widget::getDialogById(const java::String& id) const
{
    for ( long i = 0; i < dialogList.size(); i++ ) {
        WidgetDialog* dialog = dialogList.get(i);
        if ( dialog != nullptr && id.equals(dialog->getId()) ) {
            return dialog;
        }
    }
    return nullptr;
}

void Widget::addVariable(WidgetVariable* variable)
{
    variableList.add(variable);
}

WidgetVariable* Widget::getVariableByName(const java::String& name) const
{
    int i;
    for ( i = 0; i < variableList.size(); i++ ) {
        if ( variableList.get(i)->getName().equals(name) ) {
            return variableList.get(i);
        }
    }
    return nullptr;
}

java::String Widget::toString() const
{
    java::String msg = "= Widget report =========================================================\n";
    msg = msg + "Widget cache structure contains " +
        java::String::valueOf((long)popupMenuList.size()) +
        " popup submenu structures registered\n";
    msg = msg + "Widget cache structure contains " +
        java::String::valueOf((long)commandList.size()) +
        " commands registered\n";

    int i;
    for ( i = 0; i < commandList.size(); i++ ) {
        msg = msg + commandList.get(i)->toString();
    }

    if ( menubar == nullptr ) {
        msg = msg + "There is NO menubar!";
    }
    else {
        msg = msg + "There is a menubar active, called \"" +
            menubar->getName() + "\"\n";
        msg = msg + "Dumping menubar tree structure...\n";
        msg = msg + menubar->toString();
    }

    //-----------------------------------------------------------------------
    for ( i = 0; i < dialogList.size(); i++ ) {
        msg = msg + dialogList.get(i)->toString();
    }

    for ( i = 0; i < variableList.size(); i++ ) {
        msg = msg + variableList.get(i)->toString();
    }
    //-----------------------------------------------------------------------
    msg = msg + "===========================================================================\n";

    return msg;
}

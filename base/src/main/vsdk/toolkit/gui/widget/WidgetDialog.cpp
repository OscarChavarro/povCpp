#include "java/lang/String.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/gui/widget/WidgetDialog.h"

WidgetDialog::WidgetDialog()
    : id(""), name(""), orientation(ORIENTATION_VERTICAL), collapsable(false),
      titleKey(""), modal(false)
{
}

const java::String& WidgetDialog::getTitleKey() const
{
    return titleKey;
}

void WidgetDialog::setTitleKey(const java::String& titleKey)
{
    this->titleKey = titleKey;
}

bool WidgetDialog::isModal() const
{
    return modal;
}

void WidgetDialog::setModal(bool modal)
{
    this->modal = modal;
}

const java::ArrayList<WidgetDialogElement>& WidgetDialog::getElements() const
{
    return elements;
}

void WidgetDialog::addElement(const WidgetDialogElement& element)
{
    elements.add(element);
}

const WidgetDialogElement* WidgetDialog::getElementByAction(
    const java::String& action) const
{
    for ( int i = 0; i < (int)elements.size(); i++ ) {
        const WidgetDialogElement* found = elements[i].findAction(action);
        if ( found != nullptr ) {
            return found;
        }
    }
    return nullptr;
}

bool WidgetDialog::isCollapsable() const
{
    return collapsable;
}

void WidgetDialog::setCollapsable(bool collapsable)
{
    this->collapsable = collapsable;
}

java::ArrayList<java::String>& WidgetDialog::getPendingCommandNames()
{
    return pendingCommandNames;
}

void WidgetDialog::setPendingCommandNames(
    const java::ArrayList<java::String>& pendingCommandNames)
{
    this->pendingCommandNames = pendingCommandNames;
}

java::ArrayList<WidgetElement*>& WidgetDialog::getWidgetElementList()
{
    return widgetElementList;
}

void WidgetDialog::setWidgetElementList(
    const java::ArrayList<WidgetElement*>& widgetElementList)
{
    this->widgetElementList = widgetElementList;
}

const java::String& WidgetDialog::getId() const
{
    return id;
}

void WidgetDialog::setId(const java::String& id)
{
    this->id = id;
}

const java::String& WidgetDialog::getName() const
{
    return name;
}

void WidgetDialog::setName(const java::String& name)
{
    this->name = name;
}

double WidgetDialog::getOrientation() const
{
    return orientation;
}

void WidgetDialog::setOrientation(int orientation)
{
    this->orientation = orientation;
}

java::ArrayList<java::String>& WidgetDialog::getPendingVariableNames()
{
    return pendingVariableNames;
}

void WidgetDialog::setPendingVariableNames(
    const java::ArrayList<java::String>& pendingVariableNames)
{
    this->pendingVariableNames = pendingVariableNames;
}

java::ArrayList<java::String>& WidgetDialog::getPendingDialogNames()
{
    return pendingDialogNames;
}

void WidgetDialog::setPendingDialogNames(
    const java::ArrayList<java::String>& pendingDialogNames)
{
    this->pendingDialogNames = pendingDialogNames;
}

java::ArrayList<WidgetElement*>& WidgetDialog::getChildren()
{
    return widgetElementList;
}

void WidgetDialog::setChildren(
    const java::ArrayList<WidgetElement*>& widgetElementList)
{
    this->widgetElementList = widgetElementList;
}

java::ArrayList<java::String>& WidgetDialog::getPendingDialogRefNames()
{
    return pendingDialogRefNames;
}

void WidgetDialog::setPendingDialogRefNames(
    const java::ArrayList<java::String>& pendingDialogRefNames)
{
    this->pendingDialogRefNames = pendingDialogRefNames;
}

void WidgetDialog::associateVariable(const java::String& variableName)
{
    Logger::reportMessage("WidgetDialog", Logger::FATAL_ERROR,
        "associateVariable",
        java::String("Variable ") + variableName + " not found!");
}

java::String WidgetDialog::toString() const
{
    java::String msg = "";

    msg = msg + "    DIALOG: " + getId() + "\n";
    for ( int j = 0; j < widgetElementList.size(); j++ ) {
        msg = msg + "    " + widgetElementList.get(j)->toString() + "\n";
    }
    for ( int j = 0; j < pendingCommandNames.size(); j++ ) {
        msg = msg + "    Commandname: " + pendingCommandNames.get(j) + "\n";
    }

    for ( int j = 0; j < pendingDialogRefNames.size(); j++ ) {
        msg = msg + "    DialogRefNames: " + pendingDialogRefNames.get(j) + "\n";
    }

    for ( int j = 0; j < pendingVariableNames.size(); j++ ) {
        msg = msg + "    VariableNames: " + pendingVariableNames.get(j) + "\n";
    }
    return msg;
}

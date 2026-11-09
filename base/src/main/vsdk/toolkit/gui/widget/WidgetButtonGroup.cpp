#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/gui/widget/Widget.h"
#include "vsdk/toolkit/gui/widget/WidgetButtonGroup.h"

WidgetButtonGroup::WidgetButtonGroup(Widget* parent)
    : name(""), showText(false), showIcons(false), showTitle(false),
      direction(0)
{
    context = parent;
}

void WidgetButtonGroup::setShowText(bool f)
{
    showText = f;
}

void WidgetButtonGroup::setShowIcons(bool f)
{
    showIcons = f;
}

void WidgetButtonGroup::setTitle(bool f)
{
    showTitle = f;
}

void WidgetButtonGroup::setDirection(int d)
{
    direction = d;
}

int WidgetButtonGroup::getDirection() const
{
    return direction;
}

bool WidgetButtonGroup::isShowTextSet() const
{
    return showText;
}

bool WidgetButtonGroup::isShowIconsSet() const
{
    return showIcons;
}

bool WidgetButtonGroup::isShowTitleSet() const
{
    return showTitle;
}

java::ArrayList<WidgetCommand*>& WidgetButtonGroup::getCommands()
{
    return commands;
}

void WidgetButtonGroup::setName(const java::String& n)
{
    name = n;
}

const java::String& WidgetButtonGroup::getName() const
{
    return name;
}

void WidgetButtonGroup::addCommandByName(const java::String& commandName)
{
    if ( context == nullptr ) {
        return;
    }
    WidgetCommand* command = context->getCommandByName(commandName);

    if ( command != nullptr ) {
        commands.add(command);
    }
}

java::String WidgetButtonGroup::toString() const
{
    return java::String("ButtonGroup \"") + name + "\" with " +
        java::String::valueOf((long)commands.size()) + " commands\n";
}

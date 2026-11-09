#include "vsdk/toolkit/gui/widget/WidgetCommand.h"
#include "vsdk/toolkit/media/RGBAImageUncompressed.h"
#include "vsdk/toolkit/media/RGBAPixel.h"
#include "vsdk/toolkit/media/RGBImageUncompressed.h"
#include "vsdk/toolkit/media/RGBPixel.h"

WidgetCommand::WidgetCommand()
    : id(""), name(""), briefDescription(""), help(""), icon(nullptr),
      iconTransparency(nullptr), secondaryIcon(nullptr),
      secondaryIconTransparency(nullptr)
{
}

WidgetCommand::~WidgetCommand()
{
    delete icon;
    delete iconTransparency;
    delete secondaryIcon;
    delete secondaryIconTransparency;
}

const java::String& WidgetCommand::getId() const
{
    return id;
}

const java::String& WidgetCommand::getName() const
{
    return name;
}

const java::String& WidgetCommand::getBriefDescription() const
{
    return briefDescription;
}

const java::String& WidgetCommand::getHelp() const
{
    return help;
}

RGBAImageUncompressed* WidgetCommand::getIcon() const
{
    return icon;
}

RGBImageUncompressed* WidgetCommand::getIconTransparency() const
{
    return iconTransparency;
}

void WidgetCommand::applyTransparency(RGBAImageUncompressed* target,
                                      RGBImageUncompressed* mask)
{
    if ( target == nullptr || mask == nullptr ) {
        return;
    }

    int xlimit;
    int ylimit;
    int x;
    int y;

    xlimit = target->getXSize() < mask->getXSize() ?
        target->getXSize() : mask->getXSize();
    ylimit = target->getYSize() < mask->getYSize() ?
        target->getYSize() : mask->getYSize();

    RGBPixel in;
    RGBAPixel out;
    int r;
    int g;
    int b;
    int a;

    for ( y = 0; y < ylimit; y++ ) {
        for ( x = 0; x < xlimit; x++ ) {
            mask->getPixelRgb(x, y, &in);
            r = (unsigned char)in.r;
            g = (unsigned char)in.g;
            b = (unsigned char)in.b;
            a = (r + g + b) / 3;
            target->getPixelRgba(x, y, &out);
            out.a = (char)a;
            target->putPixel(x, y, &out);
        }
    }
}

void WidgetCommand::applyTransparency()
{
    applyTransparency(icon, iconTransparency);
}

void WidgetCommand::applySecondTransparency()
{
    applyTransparency(secondaryIcon, secondaryIconTransparency);
}

void WidgetCommand::setId(const java::String& i)
{
    id = i;
}

void WidgetCommand::setName(const java::String& n)
{
    name = n;
}

void WidgetCommand::setBrief(const java::String& b)
{
    briefDescription = b;
}

void WidgetCommand::setHelp(const java::String& h)
{
    help = h;
}

void WidgetCommand::appendToHelp(const java::String& h)
{
    help = help + h;
}

void WidgetCommand::setIcon(RGBAImageUncompressed* i)
{
    if ( icon != i ) {
        delete icon;
    }
    icon = i;
}

void WidgetCommand::setIconTransparency(RGBImageUncompressed* i)
{
    if ( iconTransparency != i ) {
        delete iconTransparency;
    }
    iconTransparency = i;
}

java::String WidgetCommand::toString() const
{
    java::String msg = java::String("  - Command [") + id + "]:\n";
    msg = msg + "    . Name: " + name + "\n";
    msg = msg + "    . Brief description: " + briefDescription + "\n";
    if ( icon == nullptr ) {
        msg = msg + "    . No icon image\n";
    }
    else {
        msg = msg + "    . Icon image of size (" +
            java::String::valueOf(icon->getXSize()) + ", " +
            java::String::valueOf(icon->getYSize()) + ")\n";
    }
    return msg;
}

RGBAImageUncompressed* WidgetCommand::getSecondaryIcon() const
{
    return secondaryIcon;
}

void WidgetCommand::setSecondaryIcon(RGBAImageUncompressed* secondaryIcon)
{
    if ( this->secondaryIcon != secondaryIcon ) {
        delete this->secondaryIcon;
    }
    this->secondaryIcon = secondaryIcon;
}

RGBImageUncompressed* WidgetCommand::getSecondaryIconTransparency() const
{
    return secondaryIconTransparency;
}

void WidgetCommand::setSecondaryIconTransparency(
    RGBImageUncompressed* secondaryIconTransparency)
{
    if ( this->secondaryIconTransparency != secondaryIconTransparency ) {
        delete this->secondaryIconTransparency;
    }
    this->secondaryIconTransparency = secondaryIconTransparency;
}

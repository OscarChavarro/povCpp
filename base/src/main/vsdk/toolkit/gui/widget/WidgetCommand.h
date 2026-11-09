#ifndef __WIDGET_COMMAND__
#define __WIDGET_COMMAND__

#include "vsdk/toolkit/gui/widget/WidgetElement.h"

class RGBAImageUncompressed;
class RGBImageUncompressed;

/**
This class plays a role of leaf on an n-ary tree in the composite design
pattern. A command owns its icon images.
*/
class WidgetCommand : public WidgetElement {
private:
    java::String id;
    java::String name;
    java::String briefDescription;
    java::String help; // Could contain HTML tags
    RGBAImageUncompressed* icon;
    RGBImageUncompressed* iconTransparency;
    RGBAImageUncompressed* secondaryIcon;
    RGBImageUncompressed* secondaryIconTransparency;

    static void applyTransparency(RGBAImageUncompressed* target,
                                  RGBImageUncompressed* mask);

    WidgetCommand(const WidgetCommand& other);
    WidgetCommand& operator=(const WidgetCommand& other);

public:
    WidgetCommand();
    virtual ~WidgetCommand();

    const java::String& getId() const;
    const java::String& getName() const;
    const java::String& getBriefDescription() const;
    const java::String& getHelp() const;
    RGBAImageUncompressed* getIcon() const;
    RGBImageUncompressed* getIconTransparency() const;
    void applyTransparency();
    void applySecondTransparency();
    void setId(const java::String& i);
    void setName(const java::String& n);
    void setBrief(const java::String& b);
    void setHelp(const java::String& h);
    void appendToHelp(const java::String& h);
    void setIcon(RGBAImageUncompressed* i);
    void setIconTransparency(RGBImageUncompressed* i);
    virtual java::String toString() const override;
    RGBAImageUncompressed* getSecondaryIcon() const;
    void setSecondaryIcon(RGBAImageUncompressed* secondaryIcon);
    RGBImageUncompressed* getSecondaryIconTransparency() const;
    void setSecondaryIconTransparency(
        RGBImageUncompressed* secondaryIconTransparency);
};

#endif

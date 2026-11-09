#ifndef VITRAL_FRAMEBUFFER_TOOL_BAR_H
#define VITRAL_FRAMEBUFFER_TOOL_BAR_H

#include <stdint.h>
#include "java/util/ArrayList.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferTheme.h"

class Framebuffer;
class FramebufferEvent;

// A row of push buttons with icons (or their labels when they have none), wrapping to more rows when it is too
// narrow. Icons are blended over the button face with their alpha and reduced to about two text lines. A click
// (press and release on the same button) chooses a button for takeChosen(); the button under the pointer is
// reported by getHovered(), so that the application can show its description.
class FramebufferToolBar {
public:
    FramebufferToolBar();

    void clear();
    // Adds a button. The icon is width x height 0xAARRGGBB pixels from the top row, or null.
    void add(const java::String &label, int width, int height, const uint32_t *argb);
    int count() const { return (int)buttons.size(); }
    // Lays the buttons out from (x, y) in a band of the given width. Returns the height of the band.
    int layout(Framebuffer &surface, const FramebufferTheme &theme, int x, int y, int width);
    void draw();
    // Returns true when the event was for the bar.
    bool mouseEvent(const FramebufferEvent &event);
    // The button chosen since the last call, or -1.
    int takeChosen();
    // The button under the pointer, or -1.
    int getHovered() const { return hovered; }

private:
    class Button {
    public:
        java::String label;
        int iconWidth;
        int iconHeight;
        java::ArrayList<uint32_t> icon;
        // The icon blended over the face and reduced to the button, made by layout().
        java::ArrayList<uint32_t> shown;
        int shownWidth;
        int shownHeight;
        int x;
        int y;
        int width;
        int height;
    };

    java::ArrayList<Button> buttons;
    Framebuffer *surface;
    FramebufferTheme theme;
    int barX;
    int barY;
    int barWidth;
    int barHeight;
    int armed;
    int pressed;
    int hovered;
    int chosen;

    void drawButton(int index);
    int buttonAt(int x, int y) const;
    void prepareIcon(Button &button, int size);
};

#endif

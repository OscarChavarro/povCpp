#ifndef VITRAL_FRAMEBUFFER_SIDE_PANEL_H
#define VITRAL_FRAMEBUFFER_SIDE_PANEL_H

#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferTheme.h"

class Framebuffer;
class FramebufferEvent;

// A tabbed panel of command buttons, as the side panel of the Vitral editors: a row of tabs (wrapping to more rows
// when the panel is narrow) over the page of the selected tab. A page stacks, from the top, push buttons as wide as
// the page, multi-line texts and collapsible section titles, which show or hide the entries that follow them up to
// the next section. When a page is taller than its room it scrolls: with the wheel or with its scroll bar.
//
// Pressing and releasing the first mouse button on the same button chooses it: the application takes its tag with
// takeChosen() and runs its command. The panel never runs commands itself. The tag of the button under the pointer
// is getHovered(), so the application can show its description.
//
// Everything is drawn with the Framebuffer operations and the Motif painter, so the panel works on every backend.
class FramebufferSidePanel {
public:
    FramebufferSidePanel();
    ~FramebufferSidePanel();
    FramebufferSidePanel(const FramebufferSidePanel &) = delete;
    FramebufferSidePanel &operator=(const FramebufferSidePanel &) = delete;

    // Removes the tabs and their pages.
    void clear();
    // Adds a tab with an empty page. Returns its index.
    int addTab(const java::String &label);
    int tabCount() const { return (int)tabs.size(); }
    // Removes the entries of the page of a tab, keeping the tab.
    void clearPage(int tab);
    // Adds, at the end of the page of a tab, a title that collapses and expands the entries that follow it.
    void addSection(int tab, const java::String &title, bool expanded = true);
    // Adds a push button; tag is returned by takeChosen() and getHovered().
    void addButton(int tab, const java::String &label, long tag);
    // Adds a text, one line for every '\n' separated part. Lines too long for the page are cut.
    void addText(int tab, const java::String &text);

    void selectTab(int tab);
    int getSelectedTab() const { return selectedTab; }

    // A width that holds a page of the given number of characters and its scroll bar.
    static int preferredWidth(const Framebuffer &surface, const FramebufferTheme &theme, int columns);
    // Places the panel in a rectangle of the surface. Must be called again after any change of the tabs or pages.
    void layout(Framebuffer &surface, const FramebufferTheme &theme, int x, int y, int width, int height);
    void draw();
    // Draws the page of the selected tab again (after the application changed it and laid the panel out).
    void drawPage();

    // Returns true when the event was for the panel.
    bool mouseEvent(const FramebufferEvent &event);
    // The tag of the button chosen since the last call, or -1.
    long takeChosen();
    // The tag of the button under the pointer, or -1.
    long getHovered() const;
    // True once after the user selected another tab.
    bool takeTabChanged();

private:
    enum class Kind { Section, Button, Text };

    class Entry {
    public:
        Kind kind;
        java::String text;
        long tag;
        bool expanded;
        bool shown;
        // Position in the page, before scrolling.
        int y;
        int height;
    };

    class Tab {
    public:
        java::String label;
        java::ArrayList<Entry *> entries;
        int x;
        int y;
        int width;
        int height;
        int scroll;
        Tab() : x(0), y(0), width(0), height(0), scroll(0) {}
    };

    java::ArrayList<Tab *> tabs;
    Framebuffer *surface;
    FramebufferTheme theme;
    int panelX;
    int panelY;
    int panelWidth;
    int panelHeight;
    // The room of the page, inside its bevel.
    int pageX;
    int pageY;
    int pageWidth;
    int pageHeight;
    int contentHeight;
    int barWidth;
    int selectedTab;
    int pressed;
    bool armed;
    int hovered;
    long chosen;
    bool tabChanged;
    // Dragging the slider: the pointer's distance from the slider's top, or -1.
    int dragOffset;

    Entry *newEntry(int tab, Kind kind, const java::String &text, long tag);
    void layoutPage();
    bool hasBar() const { return contentHeight > pageHeight; }
    int entryWidth() const;
    void sliderGeometry(int &sliderY, int &sliderHeight) const;
    void setScroll(int scroll);
    void drawTab(int index);
    void drawEntry(int index);
    void drawBar();
    int tabAt(int x, int y) const;
    int entryAt(int x, int y) const;
    Tab *page() const { return selectedTab >= 0 && selectedTab < (int)tabs.size() ? tabs[selectedTab] : nullptr; }
};

#endif

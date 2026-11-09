#ifndef VITRAL_FRAMEBUFFER_LIST_H
#define VITRAL_FRAMEBUFFER_LIST_H

#include "java/util/ArrayList.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferControl.h"

// A scrolled list of text rows, each one with an indentation level (to show trees) and a tag for the application.
// A click selects only its row; with multiple selection Control+click toggles a row and Shift+click selects the
// rows from the lead row to the clicked one. Up, Down, Page Up, Page Down, Home and End move the lead row and select
// it (Space toggles it with multiple selection); the wheel and the scroll bar scroll. Selection changes answer
// Changed; Enter and double clicks answer Activated.
class FramebufferList : public FramebufferControl {
public:
    // The list asks for the room of rows rows of columns characters.
    FramebufferList(int columns, int rows, bool multiple);

    void clear();
    void add(const java::String &text, int indent, long tag);
    int count() const { return (int)items.size(); }
    const java::String &textAt(int row) const { return items[row].text; }
    long tagAt(int row) const { return items[row].tag; }
    bool isSelected(int row) const { return items[row].selected; }
    void setSelected(int row, bool selected);
    // Selects only the row (none for -1), makes it the lead row and scrolls to it.
    void selectOnly(int row);
    // The row the keyboard moves from: the last one clicked or reached, or -1.
    int getLead() const { return lead; }
    // The first selected row, or -1.
    int firstSelected() const;

    int preferredWidth(const Framebuffer &surface) const override;
    int preferredHeight(const Framebuffer &surface) const override;
    bool isFocusable() const override { return true; }
    bool isStretchable() const override { return true; }
    void draw(Framebuffer &surface, const FramebufferTheme &theme, bool focused) override;
    FramebufferResponse keyPressed(const KeyEvent &event) override;
    FramebufferResponse mouseEvent(const FramebufferEvent &event) override;

private:
    class Item {
    public:
        java::String text;
        int indent;
        long tag;
        bool selected;
    };

    java::ArrayList<Item> items;
    int columns;
    int rows;
    bool multiple;
    int lead;
    int top;
    // Known after the first draw.
    int rowHeight;
    int indentWidth;
    int inset;
    int barWidth;
    // Dragging the slider: the pointer's distance from the slider's top, or -1.
    int dragOffset;

    int visibleRows() const;
    bool hasBar() const { return count() > visibleRows(); }
    void sliderGeometry(int &sliderY, int &sliderHeight) const;
    void scrollTo(int row);
    void setTop(int row);
    FramebufferResponse moveLead(int row, bool extend);
};

#endif

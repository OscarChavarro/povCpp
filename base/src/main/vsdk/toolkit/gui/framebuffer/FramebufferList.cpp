#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/gui/framebuffer/Framebuffer.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferEvent.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferList.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferPainter.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferTheme.h"

// The modifier of AWT's InputEvent that MouseEvent leaves out.
static constexpr int SHIFT_DOWN_MASK = 64;
static constexpr int WHEEL_ROWS = 3;

FramebufferList::FramebufferList(int columnCount, int rowCount, bool multipleSelection)
    : columns(columnCount), rows(rowCount), multiple(multipleSelection), lead(-1), top(0), rowHeight(16),
      indentWidth(16), inset(2), barWidth(16), dragOffset(-1) {}

void
FramebufferList::clear() {
    items.clear();
    lead = -1;
    top = 0;
}

void
FramebufferList::add(const java::String &text, int indent, long tag) {
    Item item;
    item.text = text;
    item.indent = indent;
    item.tag = tag;
    item.selected = false;
    items.add(item);
}

void
FramebufferList::setSelected(int row, bool selected) {
    if (row >= 0 && row < count()) {
        items[row].selected = selected;
    }
}

void
FramebufferList::selectOnly(int row) {
    for (long itemIndex = 0; itemIndex < items.size(); itemIndex++) {
        Item &item = items[itemIndex];
        item.selected = false;
    }
    if (row >= 0 && row < count()) {
        items[row].selected = true;
        lead = row;
        scrollTo(row);
    }
}

int
FramebufferList::firstSelected() const {
    for (int i = 0; i < count(); i++) {
        if (items[i].selected) {
            return i;
        }
    }
    return -1;
}

int
FramebufferList::preferredWidth(const Framebuffer &surface) const {
    return columns * surface.fontWidth() + 2 * surface.fontWidth();
}

int
FramebufferList::preferredHeight(const Framebuffer &surface) const {
    return rows * (surface.fontHeight() + 2) + 8;
}

int
FramebufferList::visibleRows() const {
    int n = (height - 2 * inset) / rowHeight;
    return n < 1 ? 1 : n;
}

void
FramebufferList::setTop(int row) {
    int last = count() - visibleRows();
    top = row > last ? last : row;
    top = top < 0 ? 0 : top;
}

void
FramebufferList::scrollTo(int row) {
    if (row < top) {
        setTop(row);
    } else if (row >= top + visibleRows()) {
        setTop(row - visibleRows() + 1);
    }
}

void
FramebufferList::sliderGeometry(int &sliderY, int &sliderHeight) const {
    int trough = height - 2 * inset;
    int total = count() > 0 ? count() : 1;
    sliderHeight = trough * visibleRows() / total;
    sliderHeight = sliderHeight < barWidth ? barWidth : sliderHeight;
    sliderHeight = sliderHeight > trough ? trough : sliderHeight;
    int range = count() - visibleRows();
    sliderY = y + inset + (range > 0 ? (trough - sliderHeight) * top / range : 0);
}

void
FramebufferList::draw(Framebuffer &surface, const FramebufferTheme &theme, bool focused) {
    FramebufferPainter painter(surface, theme);
    int t = theme.shadowThickness;
    rowHeight = surface.fontHeight() + 2;
    indentWidth = 2 * surface.fontWidth();
    inset = t + 1;
    barWidth = surface.fontWidth() + 2 * t + 2;
    setTop(top);
    int textRight = hasBar() ? x + width - barWidth : x + width;
    surface.beginDrawing();
    surface.fillRectangle(x, y, width, height, theme.light);
    painter.bevel(x, y, width, height, false, false);
    surface.setClip(x + inset, y + inset, textRight - x - 2 * inset, height - 2 * inset);
    int shown = visibleRows();
    for (int i = 0; i < shown && top + i < count(); i++) {
        const Item &item = items[top + i];
        int rowY = y + inset + i * rowHeight;
        uint32_t color = theme.text;
        if (item.selected) {
            surface.fillRectangle(x + inset, rowY, textRight - x - 2 * inset, rowHeight, theme.shaded ? theme.activeFace
                                                                                                      : theme.armedFace);
            color = theme.shaded ? theme.text : theme.armedText;
        }
        if (focused && top + i == lead) {
            surface.rectangle(x + inset, rowY, textRight - x - 2 * inset, rowHeight, color);
        }
        surface.text(x + inset + surface.fontWidth() / 2 + item.indent * indentWidth, rowY + 1, item.text.c_str(),
                     color);
    }
    surface.resetClip();
    if (hasBar()) {
        int barX = x + width - barWidth - inset + 1;
        surface.fillRectangle(barX, y + inset, barWidth, height - 2 * inset, theme.shaded ? theme.shadow : theme.face);
        if (!theme.shaded) {
            surface.rectangle(barX, y + inset, barWidth, height - 2 * inset, theme.outline);
        }
        int sliderY;
        int sliderHeight;
        sliderGeometry(sliderY, sliderHeight);
        painter.panel(barX, sliderY, barWidth, sliderHeight, true, false);
    }
    surface.endDrawing();
}

FramebufferResponse
FramebufferList::moveLead(int row, bool extend) {
    if (count() == 0) {
        return FramebufferResponse::Consumed;
    }
    row = row < 0 ? 0 : row >= count() ? count() - 1 : row;
    if (extend && multiple && lead >= 0) {
        int from = lead < row ? lead : row;
        int to = lead < row ? row : lead;
        for (int i = from; i <= to; i++) {
            items[i].selected = true;
        }
        lead = row;
        scrollTo(row);
    } else {
        selectOnly(row);
    }
    return FramebufferResponse::Changed;
}

FramebufferResponse
FramebufferList::keyPressed(const KeyEvent &event) {
    int key = event.keycode;
    bool extend = (event.modifierMask & KeyEvent::MASK_SHIFT) != 0;
    int page = visibleRows() > 1 ? visibleRows() - 1 : 1;
    switch (key) {
    case KeyEvent::KEY_UP:
        return moveLead(lead < 0 ? 0 : lead - 1, extend);
    case KeyEvent::KEY_DOWN:
        return moveLead(lead + 1, extend);
    case KeyEvent::KEY_PAGEUP:
        return moveLead(lead - page, extend);
    case KeyEvent::KEY_PAGEDOWN:
        return moveLead(lead < 0 ? page : lead + page, extend);
    case KeyEvent::KEY_HOME:
        return moveLead(0, extend);
    case KeyEvent::KEY_END:
        return moveLead(count() - 1, extend);
    case KeyEvent::KEY_SPACE:
        if (!multiple || lead < 0) {
            return FramebufferResponse::Ignored;
        }
        items[lead].selected = !items[lead].selected;
        return FramebufferResponse::Changed;
    case KeyEvent::KEY_ENTER:
    case KeyEvent::KEY_NUMENTER:
        return firstSelected() >= 0 ? FramebufferResponse::Activated : FramebufferResponse::Ignored;
    default:
        return FramebufferResponse::Ignored;
    }
}

FramebufferResponse
FramebufferList::mouseEvent(const FramebufferEvent &event) {
    int px = event.mouse.getX();
    int py = event.mouse.getY();
    if (event.type == FramebufferEvent::Type::MouseWheel) {
        setTop(top + WHEEL_ROWS * event.mouse.getClicks());
        return FramebufferResponse::Consumed;
    }
    if (event.type == FramebufferEvent::Type::MouseMoved) {
        if (dragOffset < 0) {
            return FramebufferResponse::Ignored;
        }
        int sliderY;
        int sliderHeight;
        sliderGeometry(sliderY, sliderHeight);
        int trough = height - 2 * inset - sliderHeight;
        int range = count() - visibleRows();
        if (trough > 0) {
            setTop((py - dragOffset - y - inset) * range / trough);
        }
        return FramebufferResponse::Consumed;
    }
    if (event.type == FramebufferEvent::Type::MouseReleased) {
        dragOffset = -1;
        return FramebufferResponse::Ignored;
    }
    if (event.type != FramebufferEvent::Type::MousePressed || event.mouse.getButton() != MouseEvent::BUTTON1) {
        return FramebufferResponse::Ignored;
    }
    if (hasBar() && px >= x + width - barWidth - inset) {
        int sliderY;
        int sliderHeight;
        sliderGeometry(sliderY, sliderHeight);
        if (py < sliderY) {
            setTop(top - visibleRows());
        } else if (py >= sliderY + sliderHeight) {
            setTop(top + visibleRows());
        } else {
            dragOffset = py - sliderY;
        }
        return FramebufferResponse::Consumed;
    }
    int row = top + (py - y - inset) / rowHeight;
    if (row < 0 || row >= count() || py < y + inset) {
        return FramebufferResponse::Consumed;
    }
    if (event.mouse.getClicks() >= 2 && items[row].selected) {
        lead = row;
        return FramebufferResponse::Activated;
    }
    int modifiers = event.mouse.getModifiers();
    if (multiple && (modifiers & MouseEvent::CTRL_DOWN_MASK)) {
        items[row].selected = !items[row].selected;
        lead = row;
        return FramebufferResponse::Changed;
    }
    return moveLead(row, (modifiers & SHIFT_DOWN_MASK) != 0);
}

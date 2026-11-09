#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/gui/framebuffer/Framebuffer.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferEvent.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferPainter.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferSidePanel.h"

// Buttons the wheel scrolls in a step.
static constexpr int WHEEL_BUTTONS = 2;

FramebufferSidePanel::FramebufferSidePanel()
    : tabs(8), surface(nullptr), theme(FramebufferTheme::monochrome()), panelX(0), panelY(0), panelWidth(0),
      panelHeight(0), pageX(0), pageY(0), pageWidth(0), pageHeight(0), contentHeight(0), barWidth(16),
      selectedTab(-1), pressed(-1), armed(false), hovered(-1), chosen(-1), tabChanged(false), dragOffset(-1) {}

FramebufferSidePanel::~FramebufferSidePanel() {
    clear();
}

void
FramebufferSidePanel::clear() {
    for (long i = 0; i < tabs.size(); i++) {
        clearPage((int)i);
        delete tabs[i];
    }
    tabs.clear();
    selectedTab = pressed = hovered = -1;
    armed = false;
    chosen = -1;
    dragOffset = -1;
    contentHeight = 0;
}

int
FramebufferSidePanel::addTab(const java::String &label) {
    Tab *tab = new Tab();
    tab->label = label;
    tabs.add(tab);
    if (selectedTab < 0) {
        selectedTab = 0;
    }
    return (int)tabs.size() - 1;
}

void
FramebufferSidePanel::clearPage(int tab) {
    if (tab < 0 || tab >= (int)tabs.size()) {
        return;
    }
    java::ArrayList<Entry *> &entries = tabs[tab]->entries;
    for (long i = 0; i < entries.size(); i++) {
        delete entries[i];
    }
    entries.clear();
    if (tab == selectedTab) {
        pressed = hovered = -1;
        armed = false;
    }
}

FramebufferSidePanel::Entry *
FramebufferSidePanel::newEntry(int tab, Kind kind, const java::String &text, long tag) {
    if (tab < 0 || tab >= (int)tabs.size()) {
        return nullptr;
    }
    Entry *entry = new Entry();
    entry->kind = kind;
    entry->text = text;
    entry->tag = tag;
    entry->expanded = true;
    entry->shown = true;
    entry->y = 0;
    entry->height = 0;
    tabs[tab]->entries.add(entry);
    return entry;
}

void
FramebufferSidePanel::addSection(int tab, const java::String &title, bool expanded) {
    Entry *entry = newEntry(tab, Kind::Section, title, -1);
    if (entry) {
        entry->expanded = expanded;
    }
}

void
FramebufferSidePanel::addButton(int tab, const java::String &label, long tag) {
    newEntry(tab, Kind::Button, label, tag);
}

void
FramebufferSidePanel::addText(int tab, const java::String &text) {
    newEntry(tab, Kind::Text, text, -1);
}

void
FramebufferSidePanel::selectTab(int tab) {
    if (tab < 0 || tab >= (int)tabs.size() || tab == selectedTab) {
        return;
    }
    selectedTab = tab;
    pressed = hovered = -1;
    armed = false;
    dragOffset = -1;
    if (surface) {
        layoutPage();
    }
}

int
FramebufferSidePanel::preferredWidth(const Framebuffer &surface, const FramebufferTheme &theme, int columns) {
    int t = theme.shadowThickness;
    int m = t + 2;
    // The margins of the panel and of the page, the bevels, and the scroll bar.
    return columns * surface.fontWidth() + 2 * m + 2 * t + 2 * (t + 1) + surface.fontWidth() + 2 * t + 2;
}

//= Layout ================================================================

void
FramebufferSidePanel::layout(Framebuffer &target, const FramebufferTheme &style, int x, int y, int width, int height) {
    surface = &target;
    theme = style;
    panelX = x;
    panelY = y;
    panelWidth = width;
    panelHeight = height;
    pressed = hovered = -1;
    armed = false;
    dragOffset = -1;
    int t = theme.shadowThickness;
    int m = t + 2;
    int fw = surface->fontWidth();
    int tabHeight = surface->fontHeight() + 2 * t + 6;
    barWidth = fw + 2 * t + 2;

    // The tabs flow from the left, in as many rows as they need.
    int tx = x + m;
    int ty = y + m;
    for (long i = 0; i < tabs.size(); i++) {
        Tab *tab = tabs[i];
        int w = surface->textWidth(tab->label.c_str(), -1) + 2 * fw;
        if (tx + w > x + width - m && tx > x + m) {
            tx = x + m;
            ty += tabHeight;
        }
        tab->x = tx;
        tab->y = ty;
        tab->width = w;
        tab->height = tabHeight;
        tx += w;
    }
    int pageTop = tabs.size() > 0 ? ty + tabHeight + 1 : y + m;
    pageX = x + m + t;
    pageY = pageTop + t;
    pageWidth = width - 2 * m - 2 * t;
    pageHeight = y + height - m - pageTop - 2 * t;
    pageWidth = pageWidth < 0 ? 0 : pageWidth;
    pageHeight = pageHeight < 0 ? 0 : pageHeight;
    layoutPage();
}

// The vertical positions of the entries of the selected page: the entries of a collapsed section are not shown.
void
FramebufferSidePanel::layoutPage() {
    contentHeight = 0;
    Tab *tab = page();
    if (!surface || !tab) {
        return;
    }
    int t = theme.shadowThickness;
    int fh = surface->fontHeight();
    int s = t + 2;
    int cy = s;
    bool open = true;
    for (long i = 0; i < tab->entries.size(); i++) {
        Entry *entry = tab->entries[i];
        entry->shown = entry->kind == Kind::Section || open;
        if (entry->kind == Kind::Section) {
            open = entry->expanded;
        }
        if (!entry->shown) {
            continue;
        }
        switch (entry->kind) {
        case Kind::Section:
            entry->height = fh + 8;
            break;
        case Kind::Button:
            entry->height = fh + 2 * t + 8;
            break;
        case Kind::Text: {
            int lines = 1;
            for (int c = 0; c < entry->text.length(); c++) {
                lines += entry->text.charAt(c) == '\n' ? 1 : 0;
            }
            entry->height = lines * fh + 4;
            break;
        }
        }
        entry->y = cy;
        cy += entry->height + s;
    }
    contentHeight = cy;
    setScroll(tab->scroll);
}

int
FramebufferSidePanel::entryWidth() const {
    int s = theme.shadowThickness + 2;
    int w = pageWidth - 2 * s - (hasBar() ? barWidth : 0);
    return w < 1 ? 1 : w;
}

void
FramebufferSidePanel::setScroll(int scroll) {
    Tab *tab = page();
    if (!tab) {
        return;
    }
    int most = contentHeight - pageHeight;
    most = most < 0 ? 0 : most;
    scroll = scroll > most ? most : scroll;
    tab->scroll = scroll < 0 ? 0 : scroll;
}

void
FramebufferSidePanel::sliderGeometry(int &sliderY, int &sliderHeight) const {
    Tab *tab = page();
    int trough = pageHeight;
    sliderHeight = contentHeight > 0 ? (int)((long)trough * pageHeight / contentHeight) : trough;
    sliderHeight = sliderHeight < barWidth ? barWidth : sliderHeight;
    sliderHeight = sliderHeight > trough ? trough : sliderHeight;
    int range = contentHeight - pageHeight;
    sliderY = pageY + (range > 0 && tab ? (int)((long)(trough - sliderHeight) * tab->scroll / range) : 0);
}

//= Drawing ===============================================================

// Narrows the clip to a rectangle inside another one.
static void
clipInside(Framebuffer &surface, int x, int y, int width, int height, int roomX, int roomY, int roomWidth,
           int roomHeight) {
    int left = x > roomX ? x : roomX;
    int top = y > roomY ? y : roomY;
    int right = x + width < roomX + roomWidth ? x + width : roomX + roomWidth;
    int bottom = y + height < roomY + roomHeight ? y + height : roomY + roomHeight;
    surface.setClip(left, top, right > left ? right - left : 0, bottom > top ? bottom - top : 0);
}

void
FramebufferSidePanel::draw() {
    if (!surface || panelWidth <= 0 || panelHeight <= 0) {
        return;
    }
    FramebufferPainter painter(*surface, theme);
    int t = theme.shadowThickness;
    surface->beginDrawing();
    painter.panel(panelX, panelY, panelWidth, panelHeight, true, false);
    for (int i = 0; i < (int)tabs.size(); i++) {
        drawTab(i);
    }
    painter.bevel(pageX - t, pageY - t, pageWidth + 2 * t, pageHeight + 2 * t, false, false);
    drawPage();
    surface->endDrawing();
}

void
FramebufferSidePanel::drawTab(int index) {
    const Tab *tab = tabs[index];
    FramebufferPainter painter(*surface, theme);
    bool current = index == selectedTab;
    surface->beginDrawing();
    surface->fillRectangle(tab->x, tab->y, tab->width, tab->height, current ? theme.armedFace : theme.face);
    painter.bevel(tab->x, tab->y, tab->width, tab->height, !current, false);
    int textWidth = surface->textWidth(tab->label.c_str(), -1);
    surface->text(tab->x + (tab->width - textWidth) / 2, tab->y + (tab->height - surface->fontHeight()) / 2,
                  tab->label.c_str(), current ? theme.armedText : theme.text);
    surface->endDrawing();
}

void
FramebufferSidePanel::drawPage() {
    if (!surface || pageWidth <= 0 || pageHeight <= 0) {
        return;
    }
    surface->beginDrawing();
    surface->fillRectangle(pageX, pageY, pageWidth, pageHeight, theme.face);
    Tab *tab = page();
    if (tab) {
        for (int i = 0; i < (int)tab->entries.size(); i++) {
            drawEntry(i);
        }
    }
    drawBar();
    surface->endDrawing();
}

void
FramebufferSidePanel::drawEntry(int index) {
    Tab *tab = page();
    if (!surface || !tab || index < 0 || index >= (int)tab->entries.size()) {
        return;
    }
    const Entry *entry = tab->entries[index];
    int roomWidth = pageWidth - (hasBar() ? barWidth : 0);
    int ex = pageX + theme.shadowThickness + 2;
    int ey = pageY + entry->y - tab->scroll;
    int w = entryWidth();
    if (!entry->shown || ey >= pageY + pageHeight || ey + entry->height <= pageY) {
        return;
    }
    FramebufferPainter painter(*surface, theme);
    int t = theme.shadowThickness;
    int fh = surface->fontHeight();
    surface->beginDrawing();
    clipInside(*surface, ex, ey, w, entry->height, pageX, pageY, roomWidth, pageHeight);
    switch (entry->kind) {
    case Kind::Button: {
        bool down = index == pressed && armed;
        painter.panel(ex, ey, w, entry->height, !down, false);
        clipInside(*surface, ex + t + 1, ey + t, w - 2 * t - 2, entry->height - 2 * t, pageX, pageY, roomWidth,
                   pageHeight);
        int textWidth = surface->textWidth(entry->text.c_str(), -1);
        int shift = down ? 1 : 0;
        int tx = ex + (w - textWidth) / 2;
        tx = tx < ex + t + 2 ? ex + t + 2 : tx;
        surface->text(tx + shift, ey + (entry->height - fh) / 2 + shift, entry->text.c_str(), theme.text);
        break;
    }
    case Kind::Section: {
        surface->fillRectangle(ex, ey, w, entry->height, theme.face);
        // A triangle pointing down when the section is expanded, right when it is collapsed.
        int size = fh / 2;
        int ax = ex + 2;
        int ay = ey + (entry->height - size) / 2;
        int xy[6];
        if (entry->expanded) {
            int down[6] = {ax, ay + size / 4, ax + size, ay + size / 4, ax + size / 2, ay + size / 4 + size / 2};
            for (int i = 0; i < 6; i++) {
                xy[i] = down[i];
            }
        } else {
            int right[6] = {ax + size / 4, ay, ax + size / 4 + size / 2, ay + size / 2, ax + size / 4, ay + size};
            for (int i = 0; i < 6; i++) {
                xy[i] = right[i];
            }
        }
        surface->fillPolygon(xy, 3, theme.text);
        surface->text(ex + size + surface->fontWidth(), ey + 3, entry->text.c_str(), theme.text);
        painter.separator(ex, ey + entry->height - 2, w);
        break;
    }
    case Kind::Text: {
        surface->fillRectangle(ex, ey, w, entry->height, theme.face);
        int start = 0;
        int line = 0;
        for (int c = 0; c <= entry->text.length(); c++) {
            if (c == entry->text.length() || entry->text.charAt(c) == '\n') {
                java::String part = entry->text.substring(start, c);
                surface->text(ex + 2, ey + 2 + line * fh, part.c_str(), theme.text);
                start = c + 1;
                line++;
            }
        }
        break;
    }
    }
    surface->resetClip();
    surface->endDrawing();
}

void
FramebufferSidePanel::drawBar() {
    if (!hasBar()) {
        return;
    }
    FramebufferPainter painter(*surface, theme);
    int barX = pageX + pageWidth - barWidth;
    surface->beginDrawing();
    surface->fillRectangle(barX, pageY, barWidth, pageHeight, theme.shaded ? theme.shadow : theme.face);
    if (!theme.shaded) {
        surface->rectangle(barX, pageY, barWidth, pageHeight, theme.outline);
    }
    int sliderY;
    int sliderHeight;
    sliderGeometry(sliderY, sliderHeight);
    painter.panel(barX, sliderY, barWidth, sliderHeight, true, false);
    surface->endDrawing();
}

//= Events ================================================================

int
FramebufferSidePanel::tabAt(int x, int y) const {
    for (int i = 0; i < (int)tabs.size(); i++) {
        const Tab *tab = tabs[i];
        if (x >= tab->x && y >= tab->y && x < tab->x + tab->width && y < tab->y + tab->height) {
            return i;
        }
    }
    return -1;
}

int
FramebufferSidePanel::entryAt(int x, int y) const {
    Tab *tab = page();
    int roomWidth = pageWidth - (hasBar() ? barWidth : 0);
    if (!tab || x < pageX || y < pageY || x >= pageX + roomWidth || y >= pageY + pageHeight) {
        return -1;
    }
    int ex = pageX + theme.shadowThickness + 2;
    if (x < ex || x >= ex + entryWidth()) {
        return -1;
    }
    int cy = y - pageY + tab->scroll;
    for (int i = 0; i < (int)tab->entries.size(); i++) {
        const Entry *entry = tab->entries[i];
        if (entry->shown && cy >= entry->y && cy < entry->y + entry->height) {
            return i;
        }
    }
    return -1;
}

bool
FramebufferSidePanel::mouseEvent(const FramebufferEvent &event) {
    Tab *tab = page();
    if (!surface) {
        return false;
    }
    int px = event.mouse.getX();
    int py = event.mouse.getY();
    bool inside = px >= panelX && py >= panelY && px < panelX + panelWidth && py < panelY + panelHeight;
    int t = theme.shadowThickness;
    int step = surface->fontHeight() + 2 * t + 8 + t + 2;
    switch (event.type) {
    case FramebufferEvent::Type::MouseWheel:
        if (!inside) {
            return false;
        }
        if (tab && hasBar()) {
            setScroll(tab->scroll + WHEEL_BUTTONS * step * event.mouse.getClicks());
            drawPage();
        }
        return true;
    case FramebufferEvent::Type::MousePressed: {
        if (!inside) {
            return false;
        }
        if (event.mouse.getButton() != MouseEvent::BUTTON1) {
            return true;
        }
        int clickedTab = tabAt(px, py);
        if (clickedTab >= 0) {
            if (clickedTab != selectedTab) {
                selectTab(clickedTab);
                tabChanged = true;
                draw();
            }
            return true;
        }
        if (!tab) {
            return true;
        }
        int barX = pageX + pageWidth - barWidth;
        if (hasBar() && px >= barX && px < pageX + pageWidth && py >= pageY && py < pageY + pageHeight) {
            int sliderY;
            int sliderHeight;
            sliderGeometry(sliderY, sliderHeight);
            if (py < sliderY) {
                setScroll(tab->scroll - pageHeight);
            } else if (py >= sliderY + sliderHeight) {
                setScroll(tab->scroll + pageHeight);
            } else {
                dragOffset = py - sliderY;
            }
            drawPage();
            return true;
        }
        int index = entryAt(px, py);
        if (index < 0) {
            return true;
        }
        Entry *entry = tab->entries[index];
        if (entry->kind == Kind::Section) {
            entry->expanded = !entry->expanded;
            layoutPage();
            drawPage();
        } else if (entry->kind == Kind::Button) {
            pressed = index;
            armed = true;
            drawEntry(index);
        }
        return true;
    }
    case FramebufferEvent::Type::MouseMoved: {
        if (dragOffset >= 0 && tab) {
            int sliderY;
            int sliderHeight;
            sliderGeometry(sliderY, sliderHeight);
            int trough = pageHeight - sliderHeight;
            int range = contentHeight - pageHeight;
            if (trough > 0) {
                setScroll((int)((long)range * (py - dragOffset - pageY) / trough));
                drawPage();
            }
            return true;
        }
        int index = inside ? entryAt(px, py) : -1;
        hovered = index >= 0 && tab && tab->entries[index]->kind == Kind::Button ? index : -1;
        if (pressed >= 0) {
            bool now = index == pressed;
            if (now != armed) {
                armed = now;
                drawEntry(pressed);
            }
            return true;
        }
        return inside;
    }
    case FramebufferEvent::Type::MouseReleased: {
        if (dragOffset >= 0) {
            dragOffset = -1;
            return true;
        }
        if (pressed < 0) {
            return inside;
        }
        int was = pressed;
        bool fire = armed && event.mouse.getButton() == MouseEvent::BUTTON1;
        pressed = -1;
        armed = false;
        drawEntry(was);
        if (fire && tab) {
            chosen = tab->entries[was]->tag;
        }
        return true;
    }
    default:
        return inside;
    }
}

long
FramebufferSidePanel::takeChosen() {
    long tag = chosen;
    chosen = -1;
    return tag;
}

long
FramebufferSidePanel::getHovered() const {
    Tab *tab = page();
    return tab && hovered >= 0 && hovered < (int)tab->entries.size() ? tab->entries[hovered]->tag : -1;
}

bool
FramebufferSidePanel::takeTabChanged() {
    bool changed = tabChanged;
    tabChanged = false;
    return changed;
}

#include <new>
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/gui/KeyEvent.h"
#include "vsdk/toolkit/gui/MouseEvent.h"
#include "vsdk/toolkit/gui/framebuffer/Framebuffer.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferEvent.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferMenuBar.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferPainter.h"
#include "vsdk/toolkit/gui/widget/WidgetMenu.h"
#include "vsdk/toolkit/gui/widget/WidgetMenuItem.h"

static constexpr int BAR_LEVEL = -1;
static constexpr int NO_LEVEL = -2;

FramebufferMenuBar::FramebufferMenuBar()
    : surface(nullptr), theme(FramebufferTheme::monochrome()), menubar(nullptr), barX(0), barY(0), barWidth(0),
      barHeight(0), barArmed(-1), active(false), popupMode(false), pressed(false), chosen(nullptr) {}

FramebufferMenuBar::~FramebufferMenuBar() {
    for (Popup *popup : popups) {
        delete popup;
    }
}

void
FramebufferMenuBar::setMenu(WidgetMenu *menu) {
    deactivate();
    menubar = menu;
    bar.clear();
}

FramebufferMenuBar::Entry
FramebufferMenuBar::describe(WidgetMenuElement *element) {
    Entry entry;
    entry.element = element;
    entry.mnemonic = 0;
    entry.submenu = element->isMenu();
    entry.separator = false;
    entry.enabled = true;
    entry.checkable = false;
    entry.checked = false;
    entry.x = 0;
    entry.y = 0;
    entry.width = 0;
    entry.height = 0;
    if (entry.submenu) {
        WidgetMenu *menu = static_cast<WidgetMenu *>(element);
        entry.label = menu->getName();
        entry.accelerator = menu->getAccelerator();
        entry.mnemonic = menu->getMnemonic();
        // An empty cascade has nothing to post.
        entry.enabled = menu->getChildren().size() > 0;
    } else {
        WidgetMenuItem *item = static_cast<WidgetMenuItem *>(element);
        entry.separator = item->isSeparator();
        if (!entry.separator) {
            entry.label = item->getName();
            entry.accelerator = item->getAccelerator();
            entry.mnemonic = item->getMnemonic();
            entry.enabled = !item->isGrayed();
            entry.checkable = item->isCheckable();
            entry.checked = item->isChecked();
        }
    }
    return entry;
}

bool
FramebufferMenuBar::selectable(const Entry &entry) {
    return !entry.separator && entry.enabled;
}

int
FramebufferMenuBar::margin() const {
    return theme.shadowThickness + 1;
}

int
FramebufferMenuBar::padding() const {
    return surface->fontWidth();
}

int
FramebufferMenuBar::entryHeight() const {
    return surface->fontHeight() + 2 * theme.shadowThickness + 4;
}

int
FramebufferMenuBar::separatorHeight() const {
    return 2 * theme.shadowThickness + 4;
}

int
FramebufferMenuBar::layout(Framebuffer &target, const FramebufferTheme &style, int x, int y, int width) {
    for (Popup *popup : popups) {
        delete popup;
    }
    popups.clear();
    active = false;
    popupMode = false;
    pressed = false;
    barArmed = -1;
    surface = &target;
    theme = style;
    bar.clear();
    barX = x;
    barY = y;
    barWidth = width;
    barHeight = 0;
    if (!menubar) {
        return 0;
    }
    int m = margin();
    int left = x + m;
    int right = x + width - m;
    int height = surface->fontHeight() + 2 * theme.shadowThickness + 4;
    int column = left;
    int row = y + m;
    java::ArrayList<WidgetMenuElement *> &children = menubar->getChildren();
    for (int i = 0; i < children.size(); i++) {
        Entry entry = describe(children.get(i));
        if (entry.separator) {
            continue;
        }
        entry.width = surface->textWidth(entry.label.c_str(), -1) + 2 * padding();
        entry.height = height;
        if (column + entry.width > right && column > left) {
            column = left;
            row += height;
        }
        entry.x = column;
        entry.y = row;
        column += entry.width;
        bar.add(entry);
    }
    // As Motif's help cascade, the last menu goes to the right end of the bar when its row has room.
    int last = (int)bar.size() - 1;
    if (last > 0 && bar[last].y == bar[last - 1].y) {
        bar[last].x = right - bar[last].width;
    }
    barHeight = (bar.isEmpty() ? height : bar[bar.size() - 1].y + height - y) + m;
    return barHeight;
}

void
FramebufferMenuBar::draw() {
    if (!surface) {
        return;
    }
    surface->beginDrawing();
    drawBar();
    for (Popup *popup : popups) {
        drawPopup(*popup);
    }
    surface->endDrawing();
}

void
FramebufferMenuBar::drawBar() {
    FramebufferPainter painter(*surface, theme);
    painter.panel(barX, barY, barWidth, barHeight, true, false);
    for (int i = 0; i < (int)bar.size(); i++) {
        drawBarEntry(i);
    }
}

void
FramebufferMenuBar::drawBarEntry(int index) {
    const Entry &entry = bar[index];
    FramebufferPainter painter(*surface, theme);
    bool armed = index == barArmed;
    surface->beginDrawing();
    surface->fillRectangle(entry.x, entry.y, entry.width, entry.height, armed ? theme.armedFace : theme.face);
    if (armed && theme.shaded) {
        painter.bevel(entry.x, entry.y, entry.width, entry.height, true, false);
    }
    painter.label(entry.x + padding(), entry.y + (entry.height - surface->fontHeight()) / 2, entry.label.c_str(),
                  entry.mnemonic, entry.enabled, armed);
    surface->endDrawing();
}

void
FramebufferMenuBar::drawPopup(const Popup &popup) {
    FramebufferPainter painter(*surface, theme);
    surface->beginDrawing();
    painter.panel(popup.x, popup.y, popup.width, popup.height, true, false);
    for (int i = 0; i < (int)popup.entries.size(); i++) {
        drawEntry(popup, i);
    }
    surface->endDrawing();
}

void
FramebufferMenuBar::drawEntry(const Popup &popup, int index) {
    const Entry &entry = popup.entries[index];
    FramebufferPainter painter(*surface, theme);
    surface->beginDrawing();
    if (entry.separator) {
        surface->fillRectangle(entry.x, entry.y, entry.width, entry.height, theme.face);
        painter.separator(entry.x, entry.y + entry.height / 2 - 1, entry.width);
        surface->endDrawing();
        return;
    }
    bool armed = index == popup.armed;
    surface->fillRectangle(entry.x, entry.y, entry.width, entry.height, armed ? theme.armedFace : theme.face);
    if (armed && theme.shaded) {
        painter.bevel(entry.x, entry.y, entry.width, entry.height, true, false);
    }
    int textY = entry.y + (entry.height - surface->fontHeight()) / 2;
    int left = entry.x + theme.shadowThickness + padding() / 2;
    bool toggles = false;
    for (const Entry &other : popup.entries) {
        toggles = toggles || other.checkable;
    }
    int box = surface->fontHeight() * 2 / 3;
    if (entry.checkable) {
        painter.toggle(left, entry.y + (entry.height - box) / 2, box, entry.checked);
    }
    if (toggles) {
        left += box + padding() / 2;
    }
    painter.label(left, textY, entry.label.c_str(), entry.mnemonic, entry.enabled, armed);
    int right = entry.x + entry.width - theme.shadowThickness - padding() / 2;
    if (entry.submenu) {
        int size = surface->fontHeight() / 2 | 1;
        right -= size;
        painter.cascadeArrow(right, entry.y + (entry.height - size) / 2, size, armed);
    } else if (!entry.accelerator.isEmpty()) {
        int width = surface->textWidth(entry.accelerator.c_str(), -1);
        uint32_t color = armed ? theme.armedText : entry.enabled ? theme.text : theme.disabledText;
        surface->text(right - width, textY, entry.accelerator.c_str(), color);
    }
    surface->endDrawing();
}

FramebufferMenuBar::Popup *
FramebufferMenuBar::openPopup(WidgetMenu *menu, int anchorX, int anchorY, int anchorWidth, int anchorHeight,
                              bool beside) {
    Popup *popup = new (std::nothrow) Popup();
    if (!popup) {
        return nullptr;
    }
    popup->menu = menu;
    popup->armed = -1;
    int m = margin();
    int labelWidth = 0;
    int acceleratorWidth = 0;
    bool cascades = false;
    bool toggles = false;
    java::ArrayList<WidgetMenuElement *> &children = menu->getChildren();
    for (int i = 0; i < children.size(); i++) {
        Entry entry = describe(children.get(i));
        int width = surface->textWidth(entry.label.c_str(), -1);
        labelWidth = width > labelWidth ? width : labelWidth;
        width = surface->textWidth(entry.accelerator.c_str(), -1);
        acceleratorWidth = width > acceleratorWidth ? width : acceleratorWidth;
        cascades = cascades || entry.submenu;
        toggles = toggles || entry.checkable;
        popup->entries.add(entry);
    }

    // The width of a column: the shadows, the check boxes, the labels, then the accelerators and cascade arrows.
    int f = padding();
    int columnWidth = 2 * theme.shadowThickness + f + labelWidth;
    if (toggles) {
        columnWidth += surface->fontHeight() * 2 / 3 + f / 2;
    }
    if (acceleratorWidth > 0) {
        columnWidth += 2 * f + acceleratorWidth;
    }
    if (cascades) {
        columnWidth += f + (surface->fontHeight() / 2 | 1);
    }

    // Menus taller than the screen continue in more columns.
    int limit = surface->height() - 2 * m;
    int columns = 1;
    int top = 0;
    int tallest = 0;
    for (Entry &entry : popup->entries) {
        entry.height = entry.separator ? separatorHeight() : entryHeight();
        if (top + entry.height > limit && top > 0) {
            columns++;
            top = 0;
        }
        entry.x = (columns - 1) * columnWidth;
        entry.y = top;
        entry.width = columnWidth;
        top += entry.height;
        tallest = top > tallest ? top : tallest;
    }
    popup->width = columns * columnWidth + 2 * m;
    popup->height = tallest + 2 * m;

    int x = beside ? anchorX + anchorWidth - theme.shadowThickness : anchorX;
    int y = beside ? anchorY - m : anchorY + anchorHeight;
    if (x + popup->width > surface->width()) {
        x = beside ? anchorX - popup->width + theme.shadowThickness : surface->width() - popup->width;
    }
    if (y + popup->height > surface->height()) {
        y = surface->height() - popup->height;
    }
    popup->x = x < 0 ? 0 : x;
    popup->y = y < 0 ? 0 : y;
    for (Entry &entry : popup->entries) {
        entry.x += popup->x + m;
        entry.y += popup->y + m;
    }
    popups.add(popup);
    surface->beginDrawing();
    popup->saved.capture(*surface, popup->x, popup->y, popup->width, popup->height);
    drawPopup(*popup);
    surface->endDrawing();
    return popup;
}

void
FramebufferMenuBar::closeFrom(int level) {
    if (level < 0) {
        level = 0;
    }
    surface->beginDrawing();
    while ((int)popups.size() > level) {
        Popup *popup = popups[popups.size() - 1];
        popups.remove(popups.size() - 1);
        popup->saved.restore();
        delete popup;
    }
    surface->endDrawing();
}

void
FramebufferMenuBar::deactivate() {
    if (surface) {
        closeFrom(0);
    }
    pressed = false;
    active = false;
    popupMode = false;
    armBar(-1);
}

void
FramebufferMenuBar::popup(WidgetMenu *menu, int x, int y) {
    if (!surface || !menu) {
        return;
    }
    deactivate();
    active = true;
    popupMode = true;
    openPopup(menu, x, y, 0, 0, false);
}

void
FramebufferMenuBar::armBar(int index) {
    if (index == barArmed) {
        return;
    }
    int old = barArmed;
    barArmed = index;
    if (!surface) {
        return;
    }
    if (old >= 0 && old < (int)bar.size()) {
        drawBarEntry(old);
    }
    if (index >= 0) {
        drawBarEntry(index);
    }
}

void
FramebufferMenuBar::arm(int level, int index) {
    Popup &popup = *popups[level];
    if (popup.armed == index) {
        return;
    }
    int old = popup.armed;
    popup.armed = index;
    if (old >= 0) {
        drawEntry(popup, old);
    }
    if (index >= 0) {
        drawEntry(popup, index);
    }
}

// Posts the menu of a cascade button of the bar, closing the menus posted before.
void
FramebufferMenuBar::post(int index, bool armFirst) {
    if (index < 0 || index >= (int)bar.size()) {
        return;
    }
    active = true;
    popupMode = false;
    closeFrom(0);
    armBar(index);
    const Entry &entry = bar[index];
    if (!entry.submenu || !entry.enabled) {
        return;
    }
    Popup *popup = openPopup(static_cast<WidgetMenu *>(entry.element), entry.x, entry.y, entry.width, entry.height,
                             false);
    if (popup && armFirst) {
        step(0, 1);
    }
}

void
FramebufferMenuBar::postSubmenu(int level, bool armFirst) {
    Popup &parent = *popups[level];
    const Entry &entry = parent.entries[parent.armed];
    if (!entry.submenu || !entry.enabled) {
        return;
    }
    if ((int)popups.size() > level + 1 && popups[level + 1]->menu == entry.element) {
        return;
    }
    closeFrom(level + 1);
    Popup *popup = openPopup(static_cast<WidgetMenu *>(entry.element), entry.x, entry.y, entry.width, entry.height,
                             true);
    if (popup && armFirst) {
        step(level + 1, 1);
    }
}

// Arms an entry under the pointer and posts it if it is a cascade.
void
FramebufferMenuBar::armAt(int level, int index) {
    Popup &popup = *popups[level];
    bool keep = index >= 0 && (int)popups.size() > level + 1 && popups[level + 1]->menu == popup.entries[index].element;
    if (!keep) {
        closeFrom(level + 1);
    }
    arm(level, index >= 0 && selectable(popup.entries[index]) ? index : -1);
    if (popup.armed >= 0) {
        postSubmenu(level, false);
    }
}

void
FramebufferMenuBar::step(int level, int direction) {
    Popup &popup = *popups[level];
    int count = (int)popup.entries.size();
    int index = popup.armed;
    for (int tries = 0; tries < count; tries++) {
        index = index < 0 && direction < 0 ? count - 1 : ((index + direction) % count + count) % count;
        if (selectable(popup.entries[index])) {
            closeFrom(level + 1);
            arm(level, index);
            return;
        }
    }
}

void
FramebufferMenuBar::choose(int level, int index) {
    if (index < 0) {
        return;
    }
    const Entry &entry = popups[level]->entries[index];
    if (!selectable(entry)) {
        return;
    }
    if (entry.submenu) {
        arm(level, index);
        postSubmenu(level, true);
        return;
    }
    chosen = static_cast<WidgetMenuItem *>(entry.element);
    deactivate();
}

bool
FramebufferMenuBar::mnemonic(char key) {
    if (key >= 'A' && key <= 'Z') {
        key = (char)(key - 'A' + 'a');
    }
    if (popups.isEmpty()) {
        for (int i = 0; i < (int)bar.size(); i++) {
            char c = bar[i].mnemonic;
            if (c >= 'A' && c <= 'Z') {
                c = (char)(c - 'A' + 'a');
            }
            if (c && c == key) {
                post(i, true);
                return true;
            }
        }
        return false;
    }
    int level = (int)popups.size() - 1;
    Popup &popup = *popups[level];
    for (int i = 0; i < (int)popup.entries.size(); i++) {
        char c = popup.entries[i].mnemonic;
        if (c >= 'A' && c <= 'Z') {
            c = (char)(c - 'A' + 'a');
        }
        if (c && c == key && selectable(popup.entries[i])) {
            arm(level, i);
            choose(level, i);
            return true;
        }
    }
    return false;
}

bool
FramebufferMenuBar::keyPressed(const KeyEvent &event) {
    if (!surface || (bar.isEmpty() && popups.isEmpty())) {
        return false;
    }
    int key = event.keycode;
    if (!active) {
        if (key != KeyEvent::KEY_F10) {
            return false;
        }
        post(0, true);
        return true;
    }
    int count = (int)bar.size();
    int level = (int)popups.size() - 1;
    if (key == KeyEvent::KEY_ESC) {
        if (popups.size() > 1) {
            closeFrom(level);
        } else {
            deactivate();
        }
    } else if (key == KeyEvent::KEY_F10) {
        deactivate();
    } else if (key == KeyEvent::KEY_UP || key == KeyEvent::KEY_DOWN) {
        if (level < 0) {
            post(barArmed, true);
        } else {
            step(level, key == KeyEvent::KEY_DOWN ? 1 : -1);
        }
    } else if (key == KeyEvent::KEY_LEFT) {
        if (level > 0) {
            closeFrom(level);
        } else if (!popupMode) {
            post((barArmed - 1 + count) % count, true);
        }
    } else if (key == KeyEvent::KEY_RIGHT) {
        Popup *deepest = level >= 0 ? popups[level] : nullptr;
        if (deepest && deepest->armed >= 0 && deepest->entries[deepest->armed].submenu &&
            deepest->entries[deepest->armed].enabled) {
            postSubmenu(level, true);
        } else if (!popupMode) {
            post((barArmed + 1) % count, true);
        }
    } else if (key == KeyEvent::KEY_ENTER || key == KeyEvent::KEY_NUMENTER || key == KeyEvent::KEY_SPACE) {
        if (level < 0) {
            post(barArmed, true);
        } else {
            choose(level, popups[level]->armed);
        }
    } else if (event.unicodeId > ' ') {
        mnemonic(event.unicodeId);
    }
    return true;
}

void
FramebufferMenuBar::locate(int x, int y, int &level, int &index) const {
    for (int l = (int)popups.size() - 1; l >= 0; l--) {
        const Popup &popup = *popups[l];
        if (x < popup.x || y < popup.y || x >= popup.x + popup.width || y >= popup.y + popup.height) {
            continue;
        }
        level = l;
        index = -1;
        for (int i = 0; i < (int)popup.entries.size(); i++) {
            const Entry &entry = popup.entries[i];
            if (x >= entry.x && y >= entry.y && x < entry.x + entry.width && y < entry.y + entry.height) {
                index = i;
            }
        }
        return;
    }
    for (int i = 0; i < (int)bar.size(); i++) {
        const Entry &entry = bar[i];
        if (x >= entry.x && y >= entry.y && x < entry.x + entry.width && y < entry.y + entry.height) {
            level = BAR_LEVEL;
            index = i;
            return;
        }
    }
    level = NO_LEVEL;
    index = -1;
}

bool
FramebufferMenuBar::mouseEvent(const FramebufferEvent &event) {
    if (!surface || (bar.isEmpty() && popups.isEmpty())) {
        return false;
    }
    int level;
    int index;
    locate(event.mouse.getX(), event.mouse.getY(), level, index);
    switch (event.type) {
    case FramebufferEvent::Type::MousePressed:
        if (event.mouse.getButton() != MouseEvent::BUTTON1) {
            return active;
        }
        if (level == BAR_LEVEL) {
            if (active && index == barArmed && !popups.isEmpty()) {
                deactivate();
            } else {
                post(index, false);
                pressed = true;
            }
            return true;
        }
        if (level >= 0) {
            armAt(level, index);
            pressed = true;
            return true;
        }
        if (active) {
            deactivate();
            return true;
        }
        return false;
    case FramebufferEvent::Type::MouseMoved:
        if (!active) {
            return false;
        }
        if (level == BAR_LEVEL && index != barArmed && !popupMode) {
            post(index, false);
        } else if (level >= 0) {
            armAt(level, index);
        }
        return true;
    case FramebufferEvent::Type::MouseReleased:
        if (event.mouse.getButton() != MouseEvent::BUTTON1 || !pressed) {
            return active;
        }
        pressed = false;
        if (level >= 0 && index >= 0 && !popups[level]->entries[index].submenu) {
            choose(level, index);
        }
        return true;
    default:
        return false;
    }
}

WidgetMenuItem *
FramebufferMenuBar::takeChosen() {
    WidgetMenuItem *item = chosen;
    chosen = nullptr;
    return item;
}

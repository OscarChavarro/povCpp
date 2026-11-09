#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/gui/framebuffer/Framebuffer.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferButton.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferDialog.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferEvent.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferInput.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferLabel.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferPainter.h"

// Stretchable controls keep room for this many text lines when the surface is small.
static constexpr int MINIMUM_STRETCHED_LINES = 3;

FramebufferDialog::FramebufferDialog(const java::String &text)
    : title(text), defaultButton(nullptr), cancelButton(nullptr), focus(nullptr), grab(nullptr), surface(nullptr),
      theme(FramebufferTheme::monochrome()), listener(nullptr), minimumColumns(0), minimumLines(0), left(0), top(0),
      width(0), height(0), buttonsY(0), running(false), result(nullptr) {}

FramebufferDialog::~FramebufferDialog() {
    for (long controlIndex = 0; controlIndex < controls.size(); controlIndex++) {
        FramebufferControl *control = controls[controlIndex];
        delete control;
    }
    for (long buttonIndex = 0; buttonIndex < buttons.size(); buttonIndex++) {
        FramebufferButton *button = buttons[buttonIndex];
        delete button;
    }
}

FramebufferControl *
FramebufferDialog::add(FramebufferControl *control) {
    if (control) {
        controls.add(control);
    }
    return control;
}

FramebufferButton *
FramebufferDialog::addButton(FramebufferButton *button, bool isDefault, bool isCancel) {
    if (!button) {
        return nullptr;
    }
    buttons.add(button);
    if (isDefault) {
        if (defaultButton) {
            defaultButton->setDefault(false);
        }
        defaultButton = button;
        button->setDefault(true);
    }
    if (isCancel) {
        cancelButton = button;
    }
    return button;
}

void
FramebufferDialog::setFocus(FramebufferControl *control) {
    FramebufferControl *old = focus;
    focus = control;
    if (running && old != control) {
        if (old) {
            drawControl(old);
        }
        if (control) {
            drawControl(control);
        }
    }
}

void
FramebufferDialog::setMinimumSize(int columns, int lines) {
    minimumColumns = columns;
    minimumLines = lines;
}

int
FramebufferDialog::margin() const {
    return surface->fontWidth();
}

int
FramebufferDialog::spacing() const {
    return surface->fontHeight() / 2;
}

bool
FramebufferDialog::isButton(const FramebufferControl *control) const {
    for (long buttonIndex = 0; buttonIndex < buttons.size(); buttonIndex++) {
        const FramebufferButton *button = buttons[buttonIndex];
        if (button == control) {
            return true;
        }
    }
    return false;
}

void
FramebufferDialog::layout() {
    int m = margin();
    int s = spacing();
    // The decoration around the client area, as the frame lays it out.
    frame.layout(*surface, theme, 0, 0, 1000, 1000);
    int decorationLeft = frame.clientX();
    int decorationTop = frame.clientY();
    int decorationWidth = 1000 - frame.clientWidth();
    int decorationHeight = 1000 - frame.clientHeight();

    int contentWidth = minimumColumns * surface->fontWidth();
    int bodyHeight = 0;
    int stretchable = 0;
    for (long controlIndex = 0; controlIndex < controls.size(); controlIndex++) {
        FramebufferControl *control = controls[controlIndex];
        int w = control->preferredWidth(*surface);
        contentWidth = w > contentWidth ? w : contentWidth;
        bodyHeight += control->preferredHeight(*surface);
        stretchable += control->isStretchable() ? 1 : 0;
    }
    bodyHeight += (controls.size() == 0) ? 0 : s * ((int)controls.size() - 1);
    int buttonWidth = 0;
    int buttonHeight = 0;
    for (long buttonIndex = 0; buttonIndex < buttons.size(); buttonIndex++) {
        FramebufferButton *button = buttons[buttonIndex];
        int w = button->preferredWidth(*surface);
        int h = button->preferredHeight(*surface);
        buttonWidth = w > buttonWidth ? w : buttonWidth;
        buttonHeight = h > buttonHeight ? h : buttonHeight;
    }
    int count = (int)buttons.size();
    int gap = 2 * m;
    int rowWidth = count * buttonWidth + (count > 0 ? (count - 1) * gap : 0);
    contentWidth = rowWidth > contentWidth ? rowWidth : contentWidth;
    int buttonsHeight = count > 0 ? 2 * s + 2 + buttonHeight : 0;

    int maximumWidth = surface->width() - decorationWidth - 2 * m - 2 * surface->fontWidth();
    int maximumHeight = surface->height() - decorationHeight - 2 * m - surface->fontHeight();
    contentWidth = contentWidth > maximumWidth ? maximumWidth : contentWidth;
    contentWidth = contentWidth < 1 ? 1 : contentWidth;
    if (count > 0 && rowWidth > contentWidth) {
        gap = m;
        buttonWidth = (contentWidth - (count - 1) * gap) / count;
        rowWidth = count * buttonWidth + (count - 1) * gap;
    }

    // The room the stretchable controls gain (up to the minimum size) or lose (down to the surface's height).
    int extra = 0;
    int wanted = minimumLines * surface->fontHeight() - bodyHeight - buttonsHeight;
    if (stretchable > 0 && wanted > 0) {
        extra = wanted;
    }
    int overflow = bodyHeight + extra + buttonsHeight - maximumHeight;
    if (overflow > 0) {
        extra -= overflow;
    }
    int contentHeight = bodyHeight + extra + buttonsHeight;

    width = contentWidth + 2 * m + decorationWidth;
    height = contentHeight + 2 * m + decorationHeight;
    left = (surface->width() - width) / 2;
    top = (surface->height() - height) / 2;
    left = left < 0 ? 0 : left;
    top = top < 0 ? 0 : top;
    frame.layout(*surface, theme, left, top, width, height);

    int cx = left + decorationLeft + m;
    int cy = top + decorationTop + m;
    int remaining = extra;
    int stretched = 0;
    for (long controlIndex = 0; controlIndex < controls.size(); controlIndex++) {
        FramebufferControl *control = controls[controlIndex];
        int h = control->preferredHeight(*surface);
        if (control->isStretchable() && stretchable > 0) {
            stretched++;
            int share = stretched == stretchable ? remaining : extra / stretchable;
            remaining -= share;
            h += share;
            int least = MINIMUM_STRETCHED_LINES * surface->fontHeight();
            h = h < least ? least : h;
        }
        control->place(cx, cy, contentWidth, h);
        cy += h + s;
    }
    buttonsY = top + decorationTop + m + contentHeight - buttonHeight;
    int bx = cx + (contentWidth - rowWidth) / 2;
    for (long buttonIndex = 0; buttonIndex < buttons.size(); buttonIndex++) {
        FramebufferButton *button = buttons[buttonIndex];
        button->place(bx, buttonsY, buttonWidth, buttonHeight);
        bx += buttonWidth + gap;
    }
}

void
FramebufferDialog::drawControl(FramebufferControl *control) {
    if (!surface || !control) {
        return;
    }
    surface->beginDrawing();
    control->draw(*surface, theme, control == focus);
    surface->endDrawing();
}

void
FramebufferDialog::drawAll() {
    FramebufferPainter painter(*surface, theme);
    surface->beginDrawing();
    frame.draw(*surface, theme, title.c_str(), true);
    surface->fillRectangle(frame.clientX(), frame.clientY(), frame.clientWidth(), frame.clientHeight(), theme.face);
    if (buttons.size() > 0) {
        int m = margin();
        painter.separator(frame.clientX() + m / 2, buttonsY - spacing() - 2, frame.clientWidth() - m);
    }
    for (long controlIndex = 0; controlIndex < controls.size(); controlIndex++) {
        FramebufferControl *control = controls[controlIndex];
        control->draw(*surface, theme, control == focus);
    }
    for (long buttonIndex = 0; buttonIndex < buttons.size(); buttonIndex++) {
        FramebufferButton *button = buttons[buttonIndex];
        button->draw(*surface, theme, button == focus);
    }
    surface->endDrawing();
}

void
FramebufferDialog::show() {
    surface->beginDrawing();
    layout();
    saved.capture(*surface, left, top, width, height);
    drawAll();
    surface->endDrawing();
}

void
FramebufferDialog::refreshBehind() {
    if (!running) {
        return;
    }
    surface->beginDrawing();
    saved.discard();
    if (listener) {
        listener->redrawBehind();
    }
    show();
    surface->endDrawing();
}

void
FramebufferDialog::redraw(FramebufferControl *control) {
    if (running) {
        drawControl(control);
    }
}

void
FramebufferDialog::close(FramebufferButton *button) {
    result = button;
    running = false;
}

void
FramebufferDialog::focusable(java::ArrayList<FramebufferControl *> &out) const {
    for (long controlIndex = 0; controlIndex < controls.size(); controlIndex++) {
        FramebufferControl *control = controls[controlIndex];
        if (control->isFocusable()) {
            out.add(control);
        }
    }
    for (long buttonIndex = 0; buttonIndex < buttons.size(); buttonIndex++) {
        FramebufferButton *button = buttons[buttonIndex];
        out.add(button);
    }
}

void
FramebufferDialog::moveFocus(int direction) {
    java::ArrayList<FramebufferControl *> order;
    focusable(order);
    if (order.size() == 0) {
        return;
    }
    int index = -1;
    for (int i = 0; i < (int)order.size(); i++) {
        if (order[i] == focus) {
            index = i;
        }
    }
    int count = (int)order.size();
    index = index < 0 ? 0 : ((index + direction) % count + count) % count;
    setFocus(order[index]);
}

FramebufferControl *
FramebufferDialog::controlAt(int px, int py) const {
    for (long controlIndex = 0; controlIndex < controls.size(); controlIndex++) {
        FramebufferControl *control = controls[controlIndex];
        if (control->contains(px, py)) {
            return control;
        }
    }
    for (long buttonIndex = 0; buttonIndex < buttons.size(); buttonIndex++) {
        FramebufferButton *button = buttons[buttonIndex];
        if (button->contains(px, py)) {
            return button;
        }
    }
    return nullptr;
}

void
FramebufferDialog::respond(FramebufferControl *control, FramebufferResponse response) {
    if (response == FramebufferResponse::Ignored || response == FramebufferResponse::Consumed) {
        return;
    }
    if (listener && listener->controlResponded(*this, control, response)) {
        return;
    }
    if (response != FramebufferResponse::Activated) {
        return;
    }
    if (isButton(control)) {
        close(static_cast<FramebufferButton *>(control));
    } else if (defaultButton) {
        close(defaultButton);
    }
}

void
FramebufferDialog::handle(const FramebufferEvent &event) {
    switch (event.type) {
    case FramebufferEvent::Type::Resized:
        refreshBehind();
        return;
    case FramebufferEvent::Type::KeyPressed: {
        int key = event.key.keycode;
        if (key == KeyEvent::KEY_TAB) {
            moveFocus(event.key.modifierMask & KeyEvent::MASK_SHIFT ? -1 : 1);
            return;
        }
        if (key == KeyEvent::KEY_ESC) {
            close(cancelButton);
            return;
        }
        FramebufferResponse response = focus ? focus->keyPressed(event.key) : FramebufferResponse::Ignored;
        if (response != FramebufferResponse::Ignored) {
            drawControl(focus);
            respond(focus, response);
        } else if ((key == KeyEvent::KEY_ENTER || key == KeyEvent::KEY_NUMENTER) && defaultButton) {
            respond(defaultButton, FramebufferResponse::Activated);
        }
        return;
    }
    case FramebufferEvent::Type::MousePressed: {
        FramebufferControl *control = controlAt(event.mouse.getX(), event.mouse.getY());
        if (!control) {
            return;
        }
        if (control->isFocusable()) {
            setFocus(control);
        }
        grab = control;
        FramebufferResponse response = control->mouseEvent(event);
        drawControl(control);
        respond(control, response);
        return;
    }
    case FramebufferEvent::Type::MouseMoved:
    case FramebufferEvent::Type::MouseReleased: {
        FramebufferControl *control = grab;
        if (event.type == FramebufferEvent::Type::MouseReleased) {
            grab = nullptr;
        }
        if (!control) {
            return;
        }
        FramebufferResponse response = control->mouseEvent(event);
        if (response != FramebufferResponse::Ignored) {
            drawControl(control);
        }
        respond(control, response);
        return;
    }
    case FramebufferEvent::Type::MouseWheel: {
        FramebufferControl *control = controlAt(event.mouse.getX(), event.mouse.getY());
        if (control && control->mouseEvent(event) != FramebufferResponse::Ignored) {
            drawControl(control);
        }
        return;
    }
    default:
        return;
    }
}

FramebufferButton *
FramebufferDialog::run(Framebuffer &target, FramebufferInput &input, const FramebufferTheme &style,
                       FramebufferDialogListener *owner) {
    surface = &target;
    theme = style;
    listener = owner;
    result = nullptr;
    grab = nullptr;
    if (!focus) {
        java::ArrayList<FramebufferControl *> order;
        focusable(order);
        focus = defaultButton;
        for (long controlIndex = 0; controlIndex < order.size(); controlIndex++) {
            FramebufferControl *control = order[controlIndex];
            if (!isButton(control)) {
                focus = control;
                break;
            }
        }
        if (!focus && order.size() > 0) {
            focus = order[0];
        }
    }
    running = true;
    show();
    FramebufferEvent event;
    while (running) {
        if (!input.waitEvent(event)) {
            result = nullptr;
            running = false;
            break;
        }
        handle(event);
    }
    surface->beginDrawing();
    saved.restore();
    surface->endDrawing();
    return result;
}

bool
FramebufferDialog::message(Framebuffer &surface, FramebufferInput &input, const FramebufferTheme &theme,
                           FramebufferDialogListener *listener, const java::String &title, const java::String &text,
                           const java::String &button) {
    FramebufferDialog dialog(title);
    dialog.setMinimumSize(30, 0);
    dialog.add(new FramebufferLabel(text));
    FramebufferButton *ok = dialog.addButton(new FramebufferButton(button, java::String("ok")), true, true);
    return dialog.run(surface, input, theme, listener) == ok;
}

#include "vsdk/toolkit/gui/framebuffer/Framebuffer.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferButton.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferEvent.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferPainter.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferTheme.h"

FramebufferButton::FramebufferButton(const java::String &text, const java::String &name)
    : label(text), action(name), defaultButton(false), armed(false), pressed(false) {}

int
FramebufferButton::preferredWidth(const Framebuffer &surface) const {
    int w = surface.textWidth(label.c_str(), -1) + 4 * surface.fontWidth();
    return w < 10 * surface.fontWidth() ? 10 * surface.fontWidth() : w;
}

int
FramebufferButton::preferredHeight(const Framebuffer &surface) const {
    return surface.fontHeight() * 3 / 2 + 8;
}

void
FramebufferButton::draw(Framebuffer &surface, const FramebufferTheme &theme, bool focused) {
    FramebufferPainter painter(surface, theme);
    int t = theme.shadowThickness;
    surface.beginDrawing();
    surface.fillRectangle(x, y, width, height, theme.face);
    // The default button's frame, then the location cursor of the focus, then the button.
    int inset = 0;
    if (defaultButton) {
        painter.bevel(x, y, width, height, false, false);
        inset += t + 1;
    }
    if (focused) {
        surface.rectangle(x + inset, y + inset, width - 2 * inset, height - 2 * inset, theme.text);
    }
    inset += 2;
    int bx = x + inset;
    int by = y + inset;
    int bw = width - 2 * inset;
    int bh = height - 2 * inset;
    painter.panel(bx, by, bw, bh, !armed, false);
    if (armed && !theme.shaded) {
        surface.fillRectangle(bx + 1, by + 1, bw - 2, bh - 2, theme.armedFace);
    }
    int textWidth = surface.textWidth(label.c_str(), -1);
    int shift = armed && theme.shaded ? 1 : 0;
    surface.setClip(bx + t, by + t, bw - 2 * t, bh - 2 * t);
    surface.text(bx + (bw - textWidth) / 2 + shift, by + (bh - surface.fontHeight()) / 2 + shift, label.c_str(),
                 armed && !theme.shaded ? theme.armedText : theme.text);
    surface.resetClip();
    surface.endDrawing();
}

FramebufferResponse
FramebufferButton::keyPressed(const KeyEvent &event) {
    int key = event.keycode;
    if (key == KeyEvent::KEY_ENTER || key == KeyEvent::KEY_NUMENTER || key == KeyEvent::KEY_SPACE) {
        return FramebufferResponse::Activated;
    }
    return FramebufferResponse::Ignored;
}

FramebufferResponse
FramebufferButton::mouseEvent(const FramebufferEvent &event) {
    bool inside = contains(event.mouse.getX(), event.mouse.getY());
    switch (event.type) {
    case FramebufferEvent::Type::MousePressed:
        if (event.mouse.getButton() != MouseEvent::BUTTON1) {
            return FramebufferResponse::Ignored;
        }
        pressed = true;
        armed = true;
        return FramebufferResponse::Consumed;
    case FramebufferEvent::Type::MouseMoved:
        if (!pressed || armed == inside) {
            return FramebufferResponse::Ignored;
        }
        armed = inside;
        return FramebufferResponse::Consumed;
    case FramebufferEvent::Type::MouseReleased:
        if (!pressed || event.mouse.getButton() != MouseEvent::BUTTON1) {
            return FramebufferResponse::Ignored;
        }
        pressed = false;
        armed = false;
        return inside ? FramebufferResponse::Activated : FramebufferResponse::Consumed;
    default:
        return FramebufferResponse::Ignored;
    }
}

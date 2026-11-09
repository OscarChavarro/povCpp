#include "vsdk/toolkit/gui/framebuffer/Framebuffer.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferEvent.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferPainter.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferText.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferTextField.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferTheme.h"

FramebufferTextField::FramebufferTextField(const java::String &text, int count)
    : value(text), columns(count), cursor(text.length()), first(0), visible(0), charWidth(8) {}

void
FramebufferTextField::setText(const java::String &text) {
    value = text;
    cursor = value.length();
    first = 0;
    scrollToCursor();
}

int
FramebufferTextField::preferredWidth(const Framebuffer &surface) const {
    return columns * surface.fontWidth() + 2 * surface.fontWidth();
}

int
FramebufferTextField::preferredHeight(const Framebuffer &surface) const {
    return surface.fontHeight() + 12;
}

void
FramebufferTextField::scrollToCursor() {
    if (visible <= 0) {
        return;
    }
    if (cursor < first) {
        first = cursor;
    }
    while (FramebufferText::characters(value.substring(first), cursor - first) >= visible) {
        first = FramebufferText::next(value, first);
    }
}

void
FramebufferTextField::draw(Framebuffer &surface, const FramebufferTheme &theme, bool focused) {
    FramebufferPainter painter(surface, theme);
    int t = theme.shadowThickness;
    int pad = surface.fontWidth() / 2 + t;
    charWidth = surface.fontWidth();
    visible = (width - 2 * pad) / charWidth;
    scrollToCursor();
    surface.beginDrawing();
    surface.fillRectangle(x, y, width, height, theme.light);
    painter.bevel(x, y, width, height, false, false);
    int textY = y + (height - surface.fontHeight()) / 2;
    surface.setClip(x + t, y + t, width - 2 * t, height - 2 * t);
    surface.text(x + pad, textY, value.c_str() + first, theme.text);
    if (focused) {
        int column = FramebufferText::characters(value.substring(first), cursor - first);
        int cx = x + pad + column * charWidth;
        surface.line(cx - 1, textY, cx - 1, textY + surface.fontHeight() - 1, theme.text);
        surface.line(cx - 3, textY, cx + 1, textY, theme.text);
        surface.line(cx - 3, textY + surface.fontHeight() - 1, cx + 1, textY + surface.fontHeight() - 1, theme.text);
    }
    surface.resetClip();
    surface.endDrawing();
}

FramebufferResponse
FramebufferTextField::keyPressed(const KeyEvent &event) {
    int key = event.keycode;
    if (key == KeyEvent::KEY_ENTER || key == KeyEvent::KEY_NUMENTER) {
        return FramebufferResponse::Activated;
    }
    if (key == KeyEvent::KEY_LEFT) {
        cursor = FramebufferText::previous(value, cursor);
    } else if (key == KeyEvent::KEY_RIGHT) {
        cursor = FramebufferText::next(value, cursor);
    } else if (key == KeyEvent::KEY_HOME) {
        cursor = 0;
    } else if (key == KeyEvent::KEY_END) {
        cursor = value.length();
    } else if (key == KeyEvent::KEY_BACKSPACE || key == KeyEvent::KEY_DELETE) {
        int from = key == KeyEvent::KEY_BACKSPACE ? FramebufferText::previous(value, cursor) : cursor;
        int to = key == KeyEvent::KEY_BACKSPACE ? cursor : FramebufferText::next(value, cursor);
        if (from == to) {
            return FramebufferResponse::Consumed;
        }
        value = value.substring(0, from) + value.substring(to);
        cursor = from;
        scrollToCursor();
        return FramebufferResponse::Changed;
    } else {
        java::String typed = (event.modifierMask & (KeyEvent::MASK_CTRL | KeyEvent::MASK_ALT))
                                 ? java::String("")
                                 : FramebufferText::fromKey(event.unicodeId);
        if (typed.isEmpty()) {
            return FramebufferResponse::Ignored;
        }
        value = value.substring(0, cursor) + typed + value.substring(cursor);
        cursor += typed.length();
        scrollToCursor();
        return FramebufferResponse::Changed;
    }
    scrollToCursor();
    return FramebufferResponse::Consumed;
}

FramebufferResponse
FramebufferTextField::mouseEvent(const FramebufferEvent &event) {
    if (event.type != FramebufferEvent::Type::MousePressed || event.mouse.getButton() != MouseEvent::BUTTON1) {
        return FramebufferResponse::Ignored;
    }
    int pad = charWidth / 2 + 2;
    int column = (event.mouse.getX() - x - pad + charWidth / 2) / charWidth;
    column = column < 0 ? 0 : column;
    int shownBefore = FramebufferText::characters(value, first);
    cursor = FramebufferText::offsetOf(value, shownBefore + column);
    return FramebufferResponse::Consumed;
}

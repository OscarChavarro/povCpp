#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/gui/framebuffer/Framebuffer.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferEvent.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferPainter.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferToolBar.h"

FramebufferToolBar::FramebufferToolBar()
    : surface(nullptr), theme(FramebufferTheme::monochrome()), barX(0), barY(0), barWidth(0), barHeight(0),
      armed(-1), pressed(-1), hovered(-1), chosen(-1) {}

void
FramebufferToolBar::clear() {
    buttons.clear();
    armed = pressed = hovered = chosen = -1;
}

void
FramebufferToolBar::add(const java::String &label, int width, int height, const uint32_t *argb) {
    Button button;
    button.label = label;
    button.iconWidth = argb && width > 0 && height > 0 ? width : 0;
    button.iconHeight = argb && width > 0 && height > 0 ? height : 0;
    if (button.iconWidth > 0) {
        long count = (long)width * height;
        button.icon.reserve(count);
        for (long i = 0; i < count; i++) {
            button.icon.add(argb[i]);
        }
    }
    button.shownWidth = 0;
    button.shownHeight = 0;
    button.x = button.y = button.width = button.height = 0;
    buttons.add(button);
}

// The icon reduced (never enlarged) to fit a size x size square, blended over the face color.
void
FramebufferToolBar::prepareIcon(Button &button, int size) {
    button.shown.clear();
    if (button.iconWidth <= 0) {
        return;
    }
    int w = button.iconWidth;
    int h = button.iconHeight;
    if (w > size || h > size) {
        if (w >= h) {
            h = h * size / w;
            w = size;
        } else {
            w = w * size / h;
            h = size;
        }
    }
    w = w < 1 ? 1 : w;
    h = h < 1 ? 1 : h;
    button.shownWidth = w;
    button.shownHeight = h;
    long count = (long)w * h;
    button.shown.reserve(count);
    for (long i = 0; i < count; i++) {
        button.shown.add(0);
    }
    uint32_t face = theme.face;
    for (int row = 0; row < h; row++) {
        for (int column = 0; column < w; column++) {
            uint32_t p = button.icon[(long)row * button.iconHeight / h * button.iconWidth +
                                     (long)column * button.iconWidth / w];
            uint32_t a = p >> 24;
            uint32_t rgb = 0;
            for (int shift = 0; shift <= 16; shift += 8) {
                uint32_t c = (p >> shift & 0xFF) * a + (face >> shift & 0xFF) * (255 - a);
                rgb |= (c / 255) << shift;
            }
            if (!theme.shaded) {
                // Monochrome: dark, opaque enough pixels are black, the rest is the white face.
                uint32_t luminance = ((rgb >> 16 & 0xFF) * 3 + (rgb >> 8 & 0xFF) * 4 + (rgb & 0xFF) * 2) / 9;
                rgb = a > 64 && luminance < 160 ? 0x000000 : 0xFFFFFF;
            }
            button.shown[(long)row * w + column] = rgb;
        }
    }
}

int
FramebufferToolBar::layout(Framebuffer &target, const FramebufferTheme &style, int x, int y, int width) {
    surface = &target;
    theme = style;
    barX = x;
    barY = y;
    barWidth = width;
    armed = pressed = hovered = -1;
    int t = theme.shadowThickness;
    int iconSize = 2 * surface->fontHeight();
    int height = iconSize + 2 * t + 6;
    int m = t + 2;
    int column = x + m;
    int row = y + m;
    for (long buttonIndex = 0; buttonIndex < buttons.size(); buttonIndex++) {
        Button &button = buttons[buttonIndex];
        prepareIcon(button, iconSize);
        int w = button.iconWidth > 0 ? height : surface->textWidth(button.label.c_str(), -1) + 2 * surface->fontWidth();
        if (column + w > x + width - m && column > x + m) {
            column = x + m;
            row += height;
        }
        button.x = column;
        button.y = row;
        button.width = w;
        button.height = height;
        column += w;
    }
    barHeight = (buttons.size() == 0) ? 0 : buttons[buttons.size() - 1].y + height + m - y;
    return barHeight;
}

void
FramebufferToolBar::drawButton(int index) {
    const Button &button = buttons[index];
    FramebufferPainter painter(*surface, theme);
    surface->beginDrawing();
    bool down = index == armed;
    painter.panel(button.x, button.y, button.width, button.height, !down, false);
    int shift = down ? 1 : 0;
    if (button.shown.size() > 0) {
        surface->putImage(button.x + (button.width - button.shownWidth) / 2 + shift,
                          button.y + (button.height - button.shownHeight) / 2 + shift, button.shownWidth,
                          button.shownHeight, button.shown.data());
    } else {
        surface->text(button.x + surface->fontWidth() + shift,
                      button.y + (button.height - surface->fontHeight()) / 2 + shift, button.label.c_str(),
                      theme.text);
    }
    surface->endDrawing();
}

void
FramebufferToolBar::draw() {
    if (!surface || barHeight <= 0) {
        return;
    }
    FramebufferPainter painter(*surface, theme);
    surface->beginDrawing();
    painter.panel(barX, barY, barWidth, barHeight, true, false);
    for (int i = 0; i < (int)buttons.size(); i++) {
        drawButton(i);
    }
    surface->endDrawing();
}

int
FramebufferToolBar::buttonAt(int x, int y) const {
    for (int i = 0; i < (int)buttons.size(); i++) {
        const Button &b = buttons[i];
        if (x >= b.x && y >= b.y && x < b.x + b.width && y < b.y + b.height) {
            return i;
        }
    }
    return -1;
}

bool
FramebufferToolBar::mouseEvent(const FramebufferEvent &event) {
    if (!surface) {
        return false;
    }
    int px = event.mouse.getX();
    int py = event.mouse.getY();
    bool inside = px >= barX && py >= barY && px < barX + barWidth && py < barY + barHeight;
    int index = buttonAt(px, py);
    switch (event.type) {
    case FramebufferEvent::Type::MousePressed:
        if (!inside || event.mouse.getButton() != MouseEvent::BUTTON1) {
            return inside;
        }
        pressed = index;
        armed = index;
        if (index >= 0) {
            drawButton(index);
        }
        return true;
    case FramebufferEvent::Type::MouseMoved: {
        hovered = index;
        if (pressed < 0) {
            return inside;
        }
        int now = index == pressed ? pressed : -1;
        if (now != armed) {
            armed = now;
            drawButton(pressed);
        }
        return true;
    }
    case FramebufferEvent::Type::MouseReleased: {
        if (pressed < 0) {
            return inside;
        }
        int was = pressed;
        pressed = -1;
        armed = -1;
        drawButton(was);
        if (index == was && event.mouse.getButton() == MouseEvent::BUTTON1) {
            chosen = was;
        }
        return true;
    }
    default:
        return inside;
    }
}

int
FramebufferToolBar::takeChosen() {
    int index = chosen;
    chosen = -1;
    return index;
}

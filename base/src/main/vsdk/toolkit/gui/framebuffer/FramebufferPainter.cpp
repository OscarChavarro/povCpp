#include "vsdk/toolkit/gui/framebuffer/Framebuffer.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferPainter.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferTheme.h"

FramebufferPainter::FramebufferPainter(Framebuffer &target, const FramebufferTheme &style)
    : surface(target), theme(style) {}

void
FramebufferPainter::bevel(int x, int y, int width, int height, bool raised, bool active) {
    if (!theme.shaded) {
        surface.rectangle(x, y, width, height, theme.outline);
        return;
    }
    uint32_t light = active ? theme.activeLight : theme.light;
    uint32_t dark = active ? theme.activeShadow : theme.shadow;
    uint32_t topLeft = raised ? light : dark;
    uint32_t bottomRight = raised ? dark : light;
    for (int i = 0; i < theme.shadowThickness && 2 * i < width && 2 * i < height; i++) {
        int right = x + width - 1 - i;
        int bottom = y + height - 1 - i;
        surface.line(x + i, y + i, right - 1, y + i, topLeft);
        surface.line(x + i, y + i, x + i, bottom - 1, topLeft);
        surface.line(x + i + 1, bottom, right, bottom, bottomRight);
        surface.line(right, y + i + 1, right, bottom, bottomRight);
    }
}

void
FramebufferPainter::panel(int x, int y, int width, int height, bool raised, bool active) {
    surface.fillRectangle(x, y, width, height, active ? theme.activeFace : theme.face);
    bevel(x, y, width, height, raised, active);
}

void
FramebufferPainter::separator(int x, int y, int width) {
    if (!theme.shaded) {
        surface.line(x, y, x + width - 1, y, theme.outline);
        return;
    }
    surface.line(x, y, x + width - 1, y, theme.shadow);
    surface.line(x, y + 1, x + width - 1, y + 1, theme.light);
}

int
FramebufferPainter::mnemonicOffset(const char *text, char mnemonic) {
    if (!mnemonic || (unsigned char)mnemonic >= 0x80) {
        return -1;
    }
    char lower = mnemonic >= 'A' && mnemonic <= 'Z' ? (char)(mnemonic - 'A' + 'a') : mnemonic;
    int fallback = -1;
    for (int i = 0; text[i]; i++) {
        if (text[i] == mnemonic) {
            return i;
        }
        char c = text[i] >= 'A' && text[i] <= 'Z' ? (char)(text[i] - 'A' + 'a') : text[i];
        if (fallback < 0 && c == lower) {
            fallback = i;
        }
    }
    return fallback;
}

void
FramebufferPainter::label(int x, int y, const char *text, char mnemonic, bool enabled, bool armed) {
    uint32_t color = armed ? theme.armedText : theme.text;
    if (!enabled && theme.shaded) {
        surface.text(x + 1, y + 1, text, theme.light);
        color = theme.disabledText;
    }
    surface.text(x, y, text, color);
    int offset = mnemonicOffset(text, mnemonic);
    if (offset < 0 || !enabled) {
        return;
    }
    int left = x + surface.textWidth(text, offset);
    int baseline = y + surface.fontHeight() - 2;
    surface.line(left, baseline, left + surface.fontWidth() - 2, baseline, color);
}

void
FramebufferPainter::cascadeArrow(int x, int y, int size, bool armed) {
    if (!theme.shaded) {
        uint32_t color = armed ? theme.armedText : theme.text;
        for (int column = 0; column < size / 2 + 1; column++) {
            surface.line(x + column, y + column, x + column, y + size - 1 - column, color);
        }
        return;
    }
    int half = size / 2;
    for (int column = 0; column <= half; column++) {
        surface.line(x + column, y + column, x + column, y + size - 1 - column, theme.shadow);
    }
    surface.line(x, y, x, y + size - 1, theme.light);
    surface.line(x, y, x + half, y + half, theme.light);
    surface.line(x + 1, y + size - 1, x + half, y + size - 1 - half, theme.outline);
}

void
FramebufferPainter::toggle(int x, int y, int size, bool checked) {
    if (!theme.shaded) {
        surface.rectangle(x, y, size, size, theme.outline);
        if (checked) {
            surface.fillRectangle(x + 2, y + 2, size - 4, size - 4, theme.text);
        }
        return;
    }
    surface.fillRectangle(x, y, size, size, checked ? theme.shadow : theme.face);
    bevel(x, y, size, size, !checked, false);
}

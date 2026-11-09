#include "vsdk/toolkit/gui/framebuffer/Framebuffer.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferPainter.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferTheme.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferWindowFrame.h"

FramebufferWindowFrame::FramebufferWindowFrame() : x(0), y(0), width(0), height(0), border(0), titleHeight(0) {}

void
FramebufferWindowFrame::layout(const Framebuffer &surface, const FramebufferTheme &theme, int left, int top, int w,
                               int h) {
    x = left;
    y = top;
    width = w;
    height = h;
    border = 2 * theme.shadowThickness + 2;
    titleHeight = surface.fontHeight() + 2 * theme.shadowThickness + 4;
}

void
FramebufferWindowFrame::draw(Framebuffer &surface, const FramebufferTheme &theme, const char *title,
                             bool active) const {
    FramebufferPainter painter(surface, theme);
    int t = theme.shadowThickness;
    surface.beginDrawing();
    // The border only: the title bar is drawn below and the client area belongs to the application.
    uint32_t face = active ? theme.activeFace : theme.face;
    surface.fillRectangle(x, y, width, border, face);
    surface.fillRectangle(x, y + height - border, width, border, face);
    surface.fillRectangle(x, y + border, border, height - 2 * border, face);
    surface.fillRectangle(x + width - border, y + border, border, height - 2 * border, face);
    painter.bevel(x, y, width, height, true, active);

    // The resize handles: etched cuts across the border, one title height away from each corner.
    int corner = border + titleHeight;
    if (theme.shaded && width > 2 * corner && height > 2 * corner) {
        uint32_t dark = active ? theme.activeShadow : theme.shadow;
        uint32_t light = active ? theme.activeLight : theme.light;
        for (int side = 0; side < 2; side++) {
            int cutX = side == 0 ? x + corner : x + width - corner;
            int cutY = side == 0 ? y + corner : y + height - corner;
            surface.line(cutX - 1, y + t, cutX - 1, y + border - 1, dark);
            surface.line(cutX, y + t, cutX, y + border - 1, light);
            surface.line(cutX - 1, y + height - border, cutX - 1, y + height - 1 - t, dark);
            surface.line(cutX, y + height - border, cutX, y + height - 1 - t, light);
            surface.line(x + t, cutY - 1, x + border - 1, cutY - 1, dark);
            surface.line(x + t, cutY, x + border - 1, cutY, light);
            surface.line(x + width - border, cutY - 1, x + width - 1 - t, cutY - 1, dark);
            surface.line(x + width - border, cutY, x + width - 1 - t, cutY, light);
        }
    }
    if (theme.shaded) {
        painter.bevel(x + border - t, y + border - t, width - 2 * (border - t), height - 2 * (border - t), false,
                      active);
    }

    int left = x + border;
    int top = y + border;
    int inner = width - 2 * border;
    int button = titleHeight;
    if (inner < 3 * button + surface.fontWidth()) {
        surface.endDrawing();
        return;
    }
    painter.panel(left, top, button, button, true, active);
    painter.panel(left + button, top, inner - 3 * button, button, true, active);
    painter.panel(left + inner - 2 * button, top, button, button, true, active);
    painter.panel(left + inner - button, top, button, button, true, active);

    // Window menu: a long raised bar; minimize: a small raised square; maximize: a large one.
    int barWidth = button * 3 / 5;
    int barHeight = button / 5 < 3 ? 3 : button / 5;
    painter.panel(left + (button - barWidth) / 2, top + (button - barHeight) / 2, barWidth, barHeight, true, active);
    int dot = button / 5 < 3 ? 3 : button / 5;
    painter.panel(left + inner - 2 * button + (button - dot) / 2, top + (button - dot) / 2, dot, dot, true, active);
    int square = button / 2;
    painter.panel(left + inner - button + (button - square) / 2, top + (button - square) / 2, square, square, true,
                  active);

    int titleLeft = left + button + t + 1;
    int titleWidth = inner - 3 * button - 2 * (t + 1);
    int textWidth = surface.textWidth(title, -1);
    int textX = titleLeft + (titleWidth - textWidth) / 2;
    if (textX < titleLeft) {
        textX = titleLeft;
    }
    surface.setClip(titleLeft, top, titleWidth, button);
    surface.text(textX, top + (button - surface.fontHeight()) / 2, title, theme.text);
    surface.resetClip();
    surface.endDrawing();
}

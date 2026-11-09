#include "vsdk/toolkit/gui/framebuffer/Framebuffer.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferLabel.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferText.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferTheme.h"

FramebufferLabel::FramebufferLabel(const java::String &text) : label(text), elideLeft(false) {}

int
FramebufferLabel::lines() const {
    int count = 1;
    for (int i = 0; i < label.length(); i++) {
        count += label.charAt(i) == '\n' ? 1 : 0;
    }
    return count;
}

int
FramebufferLabel::preferredWidth(const Framebuffer &surface) const {
    int widest = 0;
    int start = 0;
    for (int i = 0; i <= label.length(); i++) {
        if (i == label.length() || label.charAt(i) == '\n') {
            int w = surface.textWidth(label.c_str() + start, i - start);
            widest = w > widest ? w : widest;
            start = i + 1;
        }
    }
    return widest;
}

int
FramebufferLabel::preferredHeight(const Framebuffer &surface) const {
    return lines() * surface.fontHeight();
}

void
FramebufferLabel::draw(Framebuffer &surface, const FramebufferTheme &theme, bool) {
    surface.beginDrawing();
    surface.fillRectangle(x, y, width, height, theme.face);
    surface.setClip(x, y, width, height);
    int start = 0;
    int row = 0;
    for (int i = 0; i <= label.length(); i++) {
        if (i == label.length() || label.charAt(i) == '\n') {
            java::String line = label.substring(start, i);
            if (elideLeft && surface.textWidth(line.c_str(), -1) > width) {
                int cut = 0;
                java::String shown = line;
                while (cut < line.length() && surface.textWidth(shown.c_str(), -1) > width) {
                    cut = FramebufferText::next(line, cut);
                    shown = java::String("...") + line.substring(cut);
                }
                line = shown;
            }
            surface.text(x, y + row * surface.fontHeight(), line.c_str(), theme.text);
            row++;
            start = i + 1;
        }
    }
    surface.resetClip();
    surface.endDrawing();
}

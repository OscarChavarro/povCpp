#include "vsdk/toolkit/gui/framebuffer/Framebuffer.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferTheme.h"

FramebufferTheme
FramebufferTheme::decwindows(int thickness) {
    FramebufferTheme theme;
    theme.desktop = 0x4A6078;
    theme.face = 0xC9A994;
    theme.light = 0xE8D4C6;
    theme.shadow = 0x8C6E5E;
    theme.outline = 0x5A4438;
    theme.activeFace = 0xA4826F;
    theme.activeLight = 0xC9A994;
    theme.activeShadow = 0x6A5044;
    theme.text = 0x000000;
    theme.disabledText = theme.shadow;
    theme.armedFace = theme.face;
    theme.armedText = theme.text;
    theme.shaded = true;
    theme.shadowThickness = thickness;
    return theme;
}

FramebufferTheme
FramebufferTheme::monochrome() {
    FramebufferTheme theme;
    theme.desktop = 0xFFFFFF;
    theme.face = 0xFFFFFF;
    theme.light = 0xFFFFFF;
    theme.shadow = 0x000000;
    theme.outline = 0x000000;
    theme.activeFace = 0xFFFFFF;
    theme.activeLight = 0xFFFFFF;
    theme.activeShadow = 0x000000;
    theme.text = 0x000000;
    theme.disabledText = 0x000000;
    theme.armedFace = 0x000000;
    theme.armedText = 0xFFFFFF;
    theme.shaded = false;
    theme.shadowThickness = 1;
    return theme;
}

FramebufferTheme
FramebufferTheme::forSurface(const Framebuffer &surface) {
    if (surface.bitsPerPixel() <= 1) {
        return monochrome();
    }
    return decwindows(surface.width() >= 640 ? 2 : 1);
}

int
FramebufferTheme::colors(uint32_t *out, int size) const {
    const uint32_t all[] = {text, armedText, desktop, face, light, shadow, outline, activeFace, activeLight,
                            activeShadow, disabledText, armedFace};
    int count = 0;
    for (uint32_t color : all) {
        bool seen = false;
        for (int i = 0; i < count && !seen; i++) {
            seen = out[i] == color;
        }
        if (!seen && count < size) {
            out[count++] = color;
        }
    }
    return count;
}

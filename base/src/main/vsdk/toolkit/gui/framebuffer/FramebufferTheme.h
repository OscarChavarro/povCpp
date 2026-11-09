#ifndef VITRAL_FRAMEBUFFER_THEME_H
#define VITRAL_FRAMEBUFFER_THEME_H

#include <stdint.h>

class Framebuffer;

// Colors (0xRRGGBB) and metrics of the framebuffer GUI. Shaded themes draw Motif style three-dimensional shadows;
// flat themes draw one pixel outlines and show the armed menu entry in inverse video.
class FramebufferTheme {
public:
    uint32_t desktop;
    uint32_t face;
    uint32_t light;
    uint32_t shadow;
    uint32_t outline;
    uint32_t activeFace;
    uint32_t activeLight;
    uint32_t activeShadow;
    uint32_t text;
    uint32_t disabledText;
    uint32_t armedFace;
    uint32_t armedText;
    bool shaded;
    int shadowThickness;

    // DECwindows on OSF/1: tan faces on a slate blue desktop.
    static FramebufferTheme decwindows(int shadowThickness);
    // White background and black lines and text.
    static FramebufferTheme monochrome();
    // The theme the surface can show: monochrome in 1 bit per pixel, DECwindows otherwise.
    static FramebufferTheme forSurface(const Framebuffer &surface);
    // The colors of the theme, so that palette backends can reserve their entries before drawing.
    int colors(uint32_t *out, int size) const;
};

#endif

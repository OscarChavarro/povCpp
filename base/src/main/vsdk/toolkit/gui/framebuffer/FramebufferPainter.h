#ifndef VITRAL_FRAMEBUFFER_PAINTER_H
#define VITRAL_FRAMEBUFFER_PAINTER_H

class Framebuffer;
class FramebufferTheme;

// Motif style drawing primitives shared by the framebuffer controls.
class FramebufferPainter {
public:
    FramebufferPainter(Framebuffer &surface, const FramebufferTheme &theme);

    // Shadows of theme.shadowThickness pixels inside the rectangle: light on the top and left edges when raised,
    // on the bottom and right edges when sunken. A flat theme draws a one pixel outline instead.
    void bevel(int x, int y, int width, int height, bool raised, bool active);
    // The rectangle filled with the face color, then its bevel.
    void panel(int x, int y, int width, int height, bool raised, bool active);
    // An etched horizontal separator two pixels high.
    void separator(int x, int y, int width);
    // Text with the first occurrence of its mnemonic underlined; disabled text is etched.
    void label(int x, int y, const char *text, char mnemonic, bool enabled, bool armed);
    // A raised triangle pointing right, as on Motif cascade buttons.
    void cascadeArrow(int x, int y, int size, bool armed);
    // A Motif check box: sunken and filled when checked.
    void toggle(int x, int y, int size, bool checked);

private:
    Framebuffer &surface;
    const FramebufferTheme &theme;

    static int mnemonicOffset(const char *text, char mnemonic);
};

#endif

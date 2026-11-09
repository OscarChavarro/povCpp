#ifndef VITRAL_FRAMEBUFFER_WINDOW_FRAME_H
#define VITRAL_FRAMEBUFFER_WINDOW_FRAME_H

class Framebuffer;
class FramebufferTheme;

// The decoration of a top-level window as the Motif window manager draws it: a resize border, the window menu
// button, the title, and the minimize and maximize buttons. The buttons are drawn but have no behavior.
class FramebufferWindowFrame {
public:
    FramebufferWindowFrame();

    // Places the frame on the rectangle; the client area is what remains inside the border and below the title.
    void layout(const Framebuffer &surface, const FramebufferTheme &theme, int x, int y, int width, int height);
    // Draws the border and the title bar; the client area is left untouched.
    void draw(Framebuffer &surface, const FramebufferTheme &theme, const char *title, bool active) const;

    int clientX() const { return x + border; }
    int clientY() const { return y + border + titleHeight; }
    int clientWidth() const { return width - 2 * border; }
    int clientHeight() const { return height - 2 * border - titleHeight; }

private:
    int x;
    int y;
    int width;
    int height;
    int border;
    int titleHeight;
};

#endif

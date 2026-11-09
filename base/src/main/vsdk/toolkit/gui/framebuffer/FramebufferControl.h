#ifndef VITRAL_FRAMEBUFFER_CONTROL_H
#define VITRAL_FRAMEBUFFER_CONTROL_H

class Framebuffer;
class FramebufferEvent;
class FramebufferTheme;
class KeyEvent;

// What a control did with an event: nothing, something only it shows, a change of its value (a new text, a new
// selection) or an activation (a button clicked, Enter in a field, a double click on a list row).
enum class FramebufferResponse { Ignored, Consumed, Changed, Activated };

// A rectangle of a framebuffer dialog that draws itself and reacts to the keyboard while it has the focus and to
// the mouse. Controls know nothing about their dialog: they answer each event with a FramebufferResponse, and the
// dialog redraws them and tells the application.
class FramebufferControl {
public:
    FramebufferControl() : x(0), y(0), width(0), height(0) {}
    virtual ~FramebufferControl() {}

    // The size the control asks for with the surface's font.
    virtual int preferredWidth(const Framebuffer &surface) const = 0;
    virtual int preferredHeight(const Framebuffer &surface) const = 0;
    // Controls that take the keyboard focus (Tab moves it between them).
    virtual bool isFocusable() const { return false; }
    // Controls that take the room left in their dialog, such as lists and images.
    virtual bool isStretchable() const { return false; }
    virtual void draw(Framebuffer &surface, const FramebufferTheme &theme, bool focused) = 0;
    virtual FramebufferResponse keyPressed(const KeyEvent &) { return FramebufferResponse::Ignored; }
    // Mouse events in surface coordinates. A press comes when it is inside the control; the moves and the release
    // that follow it come to the same control wherever the pointer is.
    virtual FramebufferResponse mouseEvent(const FramebufferEvent &) { return FramebufferResponse::Ignored; }

    void place(int left, int top, int w, int h) {
        x = left;
        y = top;
        width = w;
        height = h;
    }
    bool contains(int px, int py) const { return px >= x && py >= y && px < x + width && py < y + height; }
    int getX() const { return x; }
    int getY() const { return y; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

protected:
    int x;
    int y;
    int width;
    int height;
};

#endif

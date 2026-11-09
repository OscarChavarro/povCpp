#ifndef VITRAL_FRAMEBUFFER_INPUT_H
#define VITRAL_FRAMEBUFFER_INPUT_H

class FramebufferEvent;
class Widget;

// Keyboard and pointer input of a framebuffer backend.
class FramebufferInput {
public:
    virtual ~FramebufferInput() {}
    // Waits for the next event. Returns false only when no more input can arrive.
    virtual bool waitEvent(FramebufferEvent &event) = 0;
    virtual bool hasPointer() const = 0;
    virtual void extendMenu(Widget &) {}
    virtual bool executeMenuCommand(const char *, const char *&) { return false; }
    // Moves the pointer to a surface position, without reporting a MouseMoved event for it. Backends that cannot
    // place the pointer ignore it and return false.
    virtual bool warpPointer(int, int) { return false; }
};

#endif

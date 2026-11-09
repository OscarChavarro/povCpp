#ifndef VITRAL_FRAMEBUFFER_EVENT_H
#define VITRAL_FRAMEBUFFER_EVENT_H

#include "vsdk/toolkit/gui/KeyEvent.h"
#include "vsdk/toolkit/gui/MouseEvent.h"

// One input event, already translated to Vitral's key and mouse events. Mouse coordinates are surface pixels;
// for MousePressed and MouseReleased the button is the one that changed, and the modifiers hold the buttons down.
// MouseWheel events carry the rotation in the clicks of the mouse event: negative away from the user, as in AWT.
// Resized means that the surface changed its size (a window backend whose window was resized): the application
// lays itself out again and redraws everything. Backends without those devices never report them.
// Any event may carry a message of the backend for the user (as the new screen mode of a Resized event), which the
// application shows where it shows its own status messages; it is null otherwise. Backends that set it own the text
// until their next event, and clear it in the events they report after that one.
class FramebufferEvent {
public:
    enum class Type { NoEvent, KeyPressed, KeyReleased, MouseMoved, MousePressed, MouseReleased, MouseWheel, Resized };

    FramebufferEvent() : type(Type::NoEvent), message(nullptr) {}

    bool isKey() const { return type == Type::KeyPressed || type == Type::KeyReleased; }
    bool isMouse() const {
        return type == Type::MouseMoved || type == Type::MousePressed || type == Type::MouseReleased ||
               type == Type::MouseWheel;
    }

    Type type;
    KeyEvent key;
    MouseEvent mouse;
    const char *message;
};

#endif

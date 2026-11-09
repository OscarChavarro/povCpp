#ifndef __MOUSE_EVENT__
#define __MOUSE_EVENT__

#include "vsdk/toolkit/gui/PresentationElement.h"
class MouseEvent : public PresentationElement {
public:
    static const int BUTTON1 = 1;
    static const int BUTTON2 = 2;
    static const int BUTTON3 = 3;
    static const int CTRL_DOWN_MASK = 128;
    static const int BUTTON1_DOWN_MASK = 1024;
    static const int BUTTON2_DOWN_MASK = 2048;
    static const int BUTTON3_DOWN_MASK = 4096;

    MouseEvent()
        : x(0), y(0), button(0), modifiers(0), clicks(0)
    {
    }

    int getX() const { return x; }
    int getY() const { return y; }
    int getButton() const { return button; }
    int getModifiers() const { return modifiers; }
    int getClicks() const { return clicks; }

    void setX(int x_val) { x = x_val; }
    void setY(int y_val) { y = y_val; }
    void setButton(int button_val) { button = button_val; }
    void setModifiers(int modifiers_val) { modifiers = modifiers_val; }
    void setClicks(int clicks_val) { clicks = clicks_val; }

private:
    int x;
    int y;
    int button;
    int modifiers;
    int clicks;
};


#endif

#ifndef VITRAL_FRAMEBUFFER_TEXT_FIELD_H
#define VITRAL_FRAMEBUFFER_TEXT_FIELD_H

#include "java/lang/String.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferControl.h"

// A one line text editor: printable keys insert at the cursor; Left, Right, Home, End, BackSpace and Delete edit;
// a click places the cursor. Every edit answers Changed; Enter answers Activated. The text scrolls to keep the
// cursor visible.
class FramebufferTextField : public FramebufferControl {
public:
    // The field asks for the width of columns characters.
    explicit FramebufferTextField(const java::String &text, int columns = 30);
    const java::String &getText() const { return value; }
    // Replaces the text and puts the cursor at its end.
    void setText(const java::String &text);

    int preferredWidth(const Framebuffer &surface) const override;
    int preferredHeight(const Framebuffer &surface) const override;
    bool isFocusable() const override { return true; }
    void draw(Framebuffer &surface, const FramebufferTheme &theme, bool focused) override;
    FramebufferResponse keyPressed(const KeyEvent &event) override;
    FramebufferResponse mouseEvent(const FramebufferEvent &event) override;

private:
    java::String value;
    int columns;
    // Byte offsets of the cursor and of the first character shown.
    int cursor;
    int first;
    // Characters that fit, known after the first draw.
    int visible;
    int charWidth;

    void scrollToCursor();
};

#endif

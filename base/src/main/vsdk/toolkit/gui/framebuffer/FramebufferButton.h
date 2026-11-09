#ifndef VITRAL_FRAMEBUFFER_BUTTON_H
#define VITRAL_FRAMEBUFFER_BUTTON_H

#include "java/lang/String.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferControl.h"

// A Motif push button: Enter or Space while it has the focus, or a click (press and release inside), activates it.
// The default button of a dialog is drawn inside a sunken frame. The action is the application's name for it.
class FramebufferButton : public FramebufferControl {
public:
    FramebufferButton(const java::String &label, const java::String &action);
    const java::String &getLabel() const { return label; }
    void setLabel(const java::String &text) { label = text; }
    const java::String &getAction() const { return action; }
    void setDefault(bool value) { defaultButton = value; }
    bool isDefault() const { return defaultButton; }

    int preferredWidth(const Framebuffer &surface) const override;
    int preferredHeight(const Framebuffer &surface) const override;
    bool isFocusable() const override { return true; }
    void draw(Framebuffer &surface, const FramebufferTheme &theme, bool focused) override;
    FramebufferResponse keyPressed(const KeyEvent &event) override;
    FramebufferResponse mouseEvent(const FramebufferEvent &event) override;

private:
    java::String label;
    java::String action;
    bool defaultButton;
    bool armed;
    bool pressed;
};

#endif

#ifndef VITRAL_FRAMEBUFFER_DIALOG_H
#define VITRAL_FRAMEBUFFER_DIALOG_H

#include "java/util/ArrayList.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferControl.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferSnapshot.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferTheme.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferWindowFrame.h"

class Framebuffer;
class FramebufferButton;
class FramebufferDialog;
class FramebufferEvent;
class FramebufferInput;

// What the application does for a modal dialog while it runs.
class FramebufferDialogListener {
public:
    virtual ~FramebufferDialogListener() {}
    // The surface changed its size, or the dialog asked for it with refreshBehind(): lay the application out again
    // and redraw it. The dialog draws itself again afterwards.
    virtual void redrawBehind() {}
    // A control of the dialog changed or was activated. Return true when the application handled it (it may close
    // the dialog with FramebufferDialog::close()); otherwise an activated button closes the dialog, and other
    // activated controls activate the default button.
    virtual bool controlResponded(FramebufferDialog &, FramebufferControl *, FramebufferResponse) { return false; }
};

// A modal dialog drawn on the surface itself, in a Motif window frame centered on it: no second native window.
// The body controls are stacked from the top and stretched to the dialog's width; the buttons are a row of equal
// buttons at the bottom, below a separator. While run() is waiting, the dialog takes every event of the input,
// as a Motif application modal dialog grabs the input of its application.
//
// Keyboard: Tab and Shift+Tab move the focus between the controls, Escape closes the dialog with its cancel button,
// Enter activates the default button when the focused control does not use it. Mouse: a press goes to the control
// under the pointer and focuses it; the moves and the release that follow go to that control; the wheel goes to the
// control under the pointer.
//
// The dialog saves the pixels it covers and puts them back when it closes; when the surface is resized, the
// listener redraws the application first. The dialog owns its controls.
class FramebufferDialog {
public:
    explicit FramebufferDialog(const java::String &title);
    ~FramebufferDialog();
    FramebufferDialog(const FramebufferDialog &) = delete;
    FramebufferDialog &operator=(const FramebufferDialog &) = delete;

    void setTitle(const java::String &text) { title = text; }
    // Adds a control below the previous ones; the dialog owns it.
    FramebufferControl *add(FramebufferControl *control);
    // Adds a button to the row at the bottom; the dialog owns it. The default button answers Enter, the cancel
    // button answers Escape and the window manager's close.
    FramebufferButton *addButton(FramebufferButton *button, bool isDefault = false, bool isCancel = false);
    FramebufferButton *getCancelButton() const { return cancelButton; }
    // Gives the keyboard focus to a control of the dialog.
    void setFocus(FramebufferControl *control);
    // Asks for a size in characters and lines of the surface's font, as a minimum; 0 for the controls' own.
    void setMinimumSize(int columns, int lines);

    // Shows the dialog and handles the events until it closes. Returns the button that closed it, or null when the
    // cancel button is missing and Escape closed it, or when the input ended.
    FramebufferButton *run(Framebuffer &surface, FramebufferInput &input, const FramebufferTheme &theme,
                           FramebufferDialogListener *listener);
    // Closes the dialog from a listener, with the button run() returns.
    void close(FramebufferButton *result);
    // Draws one control again, from a listener that changed it.
    void redraw(FramebufferControl *control);
    // Lets the listener redraw the application behind the dialog (to show the effect of a control) and draws the
    // dialog over it again.
    void refreshBehind();

    // Shows a message with one button. Returns false when the input ended.
    static bool message(Framebuffer &surface, FramebufferInput &input, const FramebufferTheme &theme,
                        FramebufferDialogListener *listener, const java::String &title, const java::String &text,
                        const java::String &button);

private:
    java::String title;
    java::ArrayList<FramebufferControl *> controls;
    java::ArrayList<FramebufferButton *> buttons;
    FramebufferButton *defaultButton;
    FramebufferButton *cancelButton;
    FramebufferControl *focus;
    FramebufferControl *grab;
    FramebufferWindowFrame frame;
    FramebufferSnapshot saved;
    Framebuffer *surface;
    FramebufferTheme theme;
    FramebufferDialogListener *listener;
    int minimumColumns;
    int minimumLines;
    int left;
    int top;
    int width;
    int height;
    int buttonsY;
    bool running;
    FramebufferButton *result;

    void layout();
    void show();
    void drawAll();
    void drawControl(FramebufferControl *control);
    int margin() const;
    int spacing() const;
    void focusable(java::ArrayList<FramebufferControl *> &out) const;
    void moveFocus(int direction);
    FramebufferControl *controlAt(int x, int y) const;
    bool isButton(const FramebufferControl *control) const;
    void respond(FramebufferControl *control, FramebufferResponse response);
    void handle(const FramebufferEvent &event);
};

#endif

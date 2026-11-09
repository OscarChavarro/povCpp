#ifndef VITRAL_FRAMEBUFFER_DIALOG_FORM_H
#define VITRAL_FRAMEBUFFER_DIALOG_FORM_H

#include "java/util/TreeMap.h"
#include "java/util/ArrayList.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferDialog.h"

class DialogTreeNode;
class FramebufferButton;
class FramebufferList;
class FramebufferTextField;
class Widget;
class WidgetDialog;
class WidgetDialogElement;

// The framebuffer presentation of a dialog definition (a WidgetDialog read from etc/gui/dialogs.json), as
// XtDialogHost and Gtk4DialogHost are for their toolkits: text inputs (with the message of their label above them)
// and trees fill the dialog from the top, buttons go to the row at its bottom. The framebuffer GUI has no context
// menus, so the action elements, which have no widget in the other toolkits, are buttons too. Rows and columns
// only group their children. The texts are the messages of the GUI context.
//
// The application owns the behavior: its listener receives every button and control response, and the dialog
// closes when a button is activated and the listener does not handle it.
class FramebufferDialogForm {
public:
    FramebufferDialogForm(const Widget &context, const WidgetDialog &definition);
    // Out of line: the maps are instantiated by the translation unit that includes TreeMap.txx.
    ~FramebufferDialogForm();
    FramebufferDialogForm(const FramebufferDialogForm &) = delete;
    FramebufferDialogForm &operator=(const FramebufferDialogForm &) = delete;

    FramebufferDialog &getDialog() { return dialog; }
    // False when the definition has elements the form cannot show (file choosers): they are left out.
    bool isComplete() const { return complete; }
    // The control of a binding or of an action, or null.
    FramebufferTextField *field(const java::String &binding) const;
    FramebufferList *tree(const java::String &binding) const;
    FramebufferButton *button(const java::String &action) const;
    // The action of a button of the form, or an empty text.
    java::String actionOf(const FramebufferControl *control) const;

    // Shows a tree in the list of a binding, expanded, without its root. The root must live while it is shown.
    void setTree(const java::String &binding, const DialogTreeNode &root);
    // The node of a row of the tree of a binding, or null.
    const DialogTreeNode *node(const java::String &binding, int row) const;
    // The row of a node in the tree of a binding, or -1.
    int rowOf(const java::String &binding, const DialogTreeNode *node) const;

    // Runs the dialog. Returns the action of the button that closed it, or an empty text.
    java::String run(Framebuffer &surface, FramebufferInput &input, const FramebufferTheme &theme,
                     FramebufferDialogListener *listener);

private:
    FramebufferDialog dialog;
    bool complete;
    java::TreeMap<java::String, FramebufferTextField *> fields;
    java::TreeMap<java::String, FramebufferList *> trees;
    java::TreeMap<java::String, java::ArrayList<const DialogTreeNode *> > rows;
    java::TreeMap<java::String, FramebufferButton *> buttons;

    void build(const Widget &context, const WidgetDialogElement &element);
    void flatten(const java::String &binding, const DialogTreeNode &node, int depth);
};

#endif

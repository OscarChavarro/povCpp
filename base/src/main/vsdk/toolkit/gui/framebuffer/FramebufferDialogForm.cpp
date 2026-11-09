#include "java/lang/String.h"
#include "java/util/TreeMap.txx"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/gui/dialog/components/DialogTreeNode.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferButton.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferDialogForm.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferLabel.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferList.h"
#include "vsdk/toolkit/gui/framebuffer/FramebufferTextField.h"
#include "vsdk/toolkit/gui/widget/Widget.h"
#include "vsdk/toolkit/gui/widget/WidgetDialog.h"
#include "vsdk/toolkit/gui/widget/WidgetDialogElement.h"

FramebufferDialogForm::FramebufferDialogForm(const Widget &context, const WidgetDialog &definition)
    : dialog(context.getMessage(definition.getTitleKey())), complete(true) {
    // The first button is the default one; a cancel or close action answers Escape (see build()).
    for (const WidgetDialogElement &element : definition.getElements()) {
        build(context, element);
    }
}

FramebufferDialogForm::~FramebufferDialogForm() {}

void
FramebufferDialogForm::build(const Widget &context, const WidgetDialogElement &element) {
    const java::String &kind = element.getKind();
    if (kind.equals("row") || kind.equals("column")) {
        for (const WidgetDialogElement &child : element.getChildren()) {
            build(context, child);
        }
    } else if (kind.equals("textInput")) {
        if (!element.getLabelKey().isEmpty()) {
            dialog.add(new FramebufferLabel(context.getMessage(element.getLabelKey())));
        }
        FramebufferTextField *field = new FramebufferTextField("", 40);
        dialog.add(field);
        fields.put(element.getBinding(), field);
    } else if (kind.equals("tree")) {
        FramebufferList *list = new FramebufferList(36, 14, true);
        dialog.add(list);
        trees.put(element.getBinding(), list);
    } else if (kind.equals("button") || kind.equals("action")) {
        bool isCancel = element.getAction().equals("cancel") || element.getAction().equals("close");
        FramebufferButton *button = new FramebufferButton(context.getMessage(element.getLabelKey()),
                                                          element.getAction());
        dialog.addButton(button, buttons.isEmpty() && !isCancel, isCancel);
        buttons.put(element.getAction(), button);
    } else {
        complete = false;
    }
}

FramebufferTextField *
FramebufferDialogForm::field(const java::String &binding) const {
    FramebufferTextField * const* i = fields.get(binding);
    return i == nullptr ? nullptr : (*i);
}

FramebufferList *
FramebufferDialogForm::tree(const java::String &binding) const {
    FramebufferList * const* i = trees.get(binding);
    return i == nullptr ? nullptr : (*i);
}

FramebufferButton *
FramebufferDialogForm::button(const java::String &action) const {
    FramebufferButton * const* i = buttons.get(action);
    return i == nullptr ? nullptr : (*i);
}

java::String
FramebufferDialogForm::actionOf(const FramebufferControl *control) const {
    for (long i = 0; i < buttons.size(); i++) {
        if (buttons.getValue(i) == control) {
            return buttons.getKey(i);
        }
    }
    return java::String("");
}

void
FramebufferDialogForm::flatten(const java::String &binding, const DialogTreeNode &node, int depth) {
    FramebufferList *list = tree(binding);
    java::ArrayList<const DialogTreeNode *> &bindingRows =
        rows.putIfAbsent(binding, java::ArrayList<const DialogTreeNode *>());
    for (const DialogTreeNode &child : node.getChildren()) {
        bindingRows.add(&child);
        list->add(child.getLabel(), depth, (long)bindingRows.size() - 1);
        flatten(binding, child, depth + 1);
    }
}

void
FramebufferDialogForm::setTree(const java::String &binding, const DialogTreeNode &root) {
    FramebufferList *list = tree(binding);
    if (!list) {
        return;
    }
    list->clear();
    rows.putIfAbsent(binding, java::ArrayList<const DialogTreeNode *>()).clear();
    flatten(binding, root, 0);
}

const DialogTreeNode *
FramebufferDialogForm::node(const java::String &binding, int row) const {
    const java::ArrayList<const DialogTreeNode *>* i = rows.get(binding);
    if (i == nullptr || row < 0 || row >= (int)(*i).size()) {
        return nullptr;
    }
    return (*i)[row];
}

int
FramebufferDialogForm::rowOf(const java::String &binding, const DialogTreeNode *target) const {
    const java::ArrayList<const DialogTreeNode *>* i = rows.get(binding);
    if (i == nullptr) {
        return -1;
    }
    for (int row = 0; row < (int)(*i).size(); row++) {
        if ((*i)[row] == target) {
            return row;
        }
    }
    return -1;
}

java::String
FramebufferDialogForm::run(Framebuffer &surface, FramebufferInput &input, const FramebufferTheme &theme,
                           FramebufferDialogListener *listener) {
    FramebufferButton *closer = dialog.run(surface, input, theme, listener);
    return closer ? actionOf(closer) : java::String("");
}

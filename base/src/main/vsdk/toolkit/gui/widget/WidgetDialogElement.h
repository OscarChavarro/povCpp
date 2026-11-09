#ifndef __WIDGET_DIALOG_ELEMENT__
#define __WIDGET_DIALOG_ELEMENT__

#include "java/util/ArrayList.h"

#include "java/lang/String.h"

/**
Ordered, toolkit-independent content of a `WidgetDialog` (the
`WidgetDialog.Element` of the Java port), as read from the dialog
definitions (`etc/gui/dialogs.json`) by `DialogPersistence`: its kind
(column, row, textInput, button, tree, action, fileChooser, fileFilter) and
the keys tying it to the messages, the values and the actions of the
application. Absent texts are empty strings (null in Java).

C++ port note: it is a class of its own, as in the TypeScript port, so it
can be used where the vitral `Widget` class and the Xt `Widget` type
collide (it does not depend on either).
*/
class WidgetDialogElement {
private:
    java::String kind;
    java::String id;
    java::String labelKey;
    java::String binding;
    java::String action;
    java::ArrayList<WidgetDialogElement> children;

public:
    // Default constructible, as the elements of `java::ArrayList`
    WidgetDialogElement() {}
    WidgetDialogElement(const java::String& kind, const java::String& id,
                        const java::String& labelKey,
                        const java::String& binding,
                        const java::String& action,
                        const java::ArrayList<WidgetDialogElement>& children);

    const java::String& getKind() const;
    const java::String& getId() const;
    const java::String& getLabelKey() const;
    const java::String& getBinding() const;
    const java::String& getAction() const;
    const java::ArrayList<WidgetDialogElement>& getChildren() const;

    /**
    @return this element or the first of its descendants with the given
    action, or null
    */
    const WidgetDialogElement* findAction(const java::String& action) const;
};

#endif

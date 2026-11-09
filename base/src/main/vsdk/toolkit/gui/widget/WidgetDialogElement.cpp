#include "java/lang/String.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/gui/widget/WidgetDialogElement.h"

WidgetDialogElement::WidgetDialogElement(
    const java::String& kind, const java::String& id,
    const java::String& labelKey, const java::String& binding,
    const java::String& action,
    const java::ArrayList<WidgetDialogElement>& children)
    : kind(kind), id(id), labelKey(labelKey), binding(binding),
      action(action), children(children)
{
}

const java::String& WidgetDialogElement::getKind() const
{
    return kind;
}

const java::String& WidgetDialogElement::getId() const
{
    return id;
}

const java::String& WidgetDialogElement::getLabelKey() const
{
    return labelKey;
}

const java::String& WidgetDialogElement::getBinding() const
{
    return binding;
}

const java::String& WidgetDialogElement::getAction() const
{
    return action;
}

const java::ArrayList<WidgetDialogElement>& WidgetDialogElement::getChildren() const
{
    return children;
}

const WidgetDialogElement* WidgetDialogElement::findAction(
    const java::String& action) const
{
    if ( !action.isEmpty() && this->action.equals(action) ) {
        return this;
    }
    for ( int i = 0; i < (int)children.size(); i++ ) {
        const WidgetDialogElement* found = children[i].findAction(action);
        if ( found != nullptr ) {
            return found;
        }
    }
    return nullptr;
}

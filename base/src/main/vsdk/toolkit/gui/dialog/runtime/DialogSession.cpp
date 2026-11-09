#include "java/lang/IllegalArgumentException.h"
#include "java/lang/String.h"
#include "java/util/TreeMap.txx"

#include "vsdk/toolkit/gui/dialog/runtime/DialogSession.h"

DialogSession::DialogSession(const WidgetDialog* definition)
    : definition(definition), open(true), busy(false), generation(0)
{
    if ( definition == nullptr ) {
        throw java::IllegalArgumentException("Missing dialog definition");
    }
}

const WidgetDialog* DialogSession::getDefinition() const
{
    return definition;
}

void DialogSession::setDraft(const java::String& binding,
                             const java::String& value)
{
    drafts.put(binding, value);
}

java::String DialogSession::getDraft(const java::String& binding) const
{
    const java::String* draft = drafts.get(binding);
    return draft != nullptr ? (*draft) : java::String("");
}

bool DialogSession::isOpen() const
{
    return open;
}

bool DialogSession::isBusy() const
{
    return busy;
}

long DialogSession::beginAction()
{
    if ( !open || busy ) {
        return -1;
    }
    busy = true;
    return ++generation;
}

bool DialogSession::completeAction(long request)
{
    if ( !open || !busy || request != generation ) {
        return false;
    }
    busy = false;
    return true;
}

void DialogSession::close()
{
    open = false;
    busy = false;
    ++generation;
}

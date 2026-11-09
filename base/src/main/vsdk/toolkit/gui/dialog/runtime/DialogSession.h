#ifndef __DIALOG_SESSION__
#define __DIALOG_SESSION__

#include "java/util/TreeMap.h"

#include "java/lang/String.h"

class WidgetDialog;

/**
Per-opening values and lifetime for a dialog definition: the drafts of its
bound values and the request of its running action, so the answer of an
action that ended after the dialog closed (or after a newer request) is
ignored.
*/
class DialogSession {
private:
    const WidgetDialog* definition;
    java::TreeMap<java::String, java::String> drafts;
    bool open;
    bool busy;
    long generation;

public:
    /**
    @param definition the dialog (referenced, it must not be null)
    */
    explicit DialogSession(const WidgetDialog* definition);

    const WidgetDialog* getDefinition() const;
    void setDraft(const java::String& binding, const java::String& value);

    /**
    @return the draft of the binding, or an empty string
    */
    java::String getDraft(const java::String& binding) const;
    bool isOpen() const;
    bool isBusy() const;

    /**
    @return identifier of the action started, or -1 if the session is
    closed or already running one
    */
    long beginAction();

    /**
    @return true if the request is the running action of the open session
    (then the session is not busy anymore)
    */
    bool completeAction(long request);
    void close();
};

#endif

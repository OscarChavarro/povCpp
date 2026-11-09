#ifndef __DIALOG_PERSISTENCE__
#define __DIALOG_PERSISTENCE__

#include "java/lang/IllegalArgumentException.h"
#include "java/lang/String.h"

class Widget;

/**
Reads the shared, ordered dialog definitions (`etc/gui/dialogs.json`)
without toolkit dependencies, as `DialogPersistence` in the Java and
TypeScript ports: each dialog becomes a `WidgetDialog` of the GUI context,
with its title key, its modality and its `WidgetDialogElement`s.

The definitions are validated as in Java: schema version 1, unique dialog
and element identifiers, known element kinds, and the bindings and actions
each kind needs. Failures are `java::IllegalArgumentException` exceptions with the
messages of the Java `IllegalArgumentException`s.

C++ port note: it takes the JSON text instead of an `InputStream`, and
reads it with its own small JSON reader (the C++ toolkit has no JSON
library).
*/
class DialogPersistence {
public:
    /**
    @param source JSON text of the dialog definitions
    @param widget GUI context receiving the dialogs (it owns them)
    */
    static void importDialogs(const java::String& source, Widget* widget);

private:
    DialogPersistence();
};

#endif

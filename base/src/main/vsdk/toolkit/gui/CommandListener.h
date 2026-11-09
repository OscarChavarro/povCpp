#ifndef __COMMAND_LISTENER__
#define __COMMAND_LISTENER__

#include "java/lang/String.h"
#include "vsdk/toolkit/gui/PresentationElement.h"

/**
This class represents the concept of event language as explained in section
[FOLE1992.10.6] and figure [FOLE1992.10.24].
*/
class CommandListener : public PresentationElement {
public:
    virtual ~CommandListener() {}
    virtual bool executeCommand(const java::String& commandId) = 0;
};

#endif

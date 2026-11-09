#ifndef __GIZMO__
#define __GIZMO__

#include "vsdk/toolkit/gui/PresentationElement.h"

/**
The Gizmo abstract class provides an interface for *Gizmo
style classes. This serves two purposes:
  - To help in design level organization of renderers (this eases the
    study of the class hierarchy)
  - To provide a place to locate possible future operations, common to
    all gizmos (but none of these as been detected yet)
*/
class Gizmo : public PresentationElement {
public:
    virtual ~Gizmo() {}
};

#endif

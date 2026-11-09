#ifndef __VIEWPORT_SET_INTERACTION_TECHNIQUES__
#define __VIEWPORT_SET_INTERACTION_TECHNIQUES__

#include "java/lang/String.h"
#include "vsdk/toolkit/gui/KeyEvent.h"
#include "vsdk/toolkit/gui/MouseEvent.h"

class Viewport;
class ViewportSet;
class ViewportSetInteractionListener;

/**
Interaction techniques over a `ViewportSet`: selection of the viewport under
the pointer, layout control and per-viewport display commands. It processes
only vitral events, so callers must convert events from the GUI technology
in use before calling it.

Mouse events must have coordinates in pixels of the `ViewportSet` area, with
origin at its upper left corner.

Mouse: pressing over a viewport selects it. If it was already selected,
pressing and releasing over its title requests the menu to change its
projection location, through the `ViewportSetInteractionListener`.

Keyboard commands:
  - `;` selects the next viewport, `,` selects the next layout style (the
    last one shows only the selected viewport) and Alt+`w` maximizes /
    restores the selected viewport.
  - Over the selected viewport: `g` toggles the grid, `t`, `l`, `f`, `b` and
    `p` select the Top, Left, Front, Bottom and Perspective cameras, `.` (and
    `9`) toggles the render mode between GPU (z-buffer) and CPU (raytracing)
    and `0` cycles the requested size.
*/
class ViewportSetInteractionTechniques {
private:
    ViewportSet* viewportSet;
    ViewportSetInteractionListener* listener;
    bool titlePressArmed;

    bool processSelectedViewportCommand(const KeyEvent& event);
    bool selectViewportUnderPointer(const MouseEvent& event);
    bool isOverTitle(const MouseEvent& event, const Viewport* viewport) const;

public:
    explicit ViewportSetInteractionTechniques(ViewportSet* viewportSet);
    virtual ~ViewportSetInteractionTechniques() {}

    /**
    @param listener who receives the requests derived from interaction, or
    null for none
    */
    void setListener(ViewportSetInteractionListener* listener);
    ViewportSet* getViewportSet() const;

    /**
    Processes one of the standard commands of the viewport set (see
    `ViewportSetCommands`), i.e. from its popup menus, over the selected
    viewport.
    @param command the id of the command, starting with `IDV_`
    @return true if the command was a viewport set one and was processed
    */
    bool processCommand(const java::String& command);

    /**
    Processes one of the standard commands of the viewport set (projection
    location or render mode) over the given viewport.
    @return true if the command was a viewport set one and was processed
    */
    bool processCommand(const java::String& command, Viewport* viewport);

    /**
    @return true if the event was a viewport set command and was processed
    */
    bool processKeyPressedEvent(const KeyEvent& event);

    /**
    @return the active viewport under the pointer, or null if none
    */
    Viewport* findViewportAt(const MouseEvent& event) const;

    /**
    @param event pointer position, in the `ViewportSet` area
    @return true if the pointer is over the title (HUD) of a viewport, that
    is, over an area where clicking has an effect
    */
    bool isPointerOverTitle(const MouseEvent& event) const;

    /**
    Selects the viewport under the pointer. If it was already the selected
    one and the press is over its title, the following click over the title
    will request the projection location menu.
    @return true if there is a viewport under the pointer
    */
    bool processMousePressedEvent(const MouseEvent& event);

    /**
    Selects the viewport under the pointer. If the press was over the title
    of the already selected viewport and the button is released over it too,
    the projection location menu is requested. (It is done on the release,
    and not on the click event, because GUI technologies do not generate
    clicks if the pointer moves slightly between the press and the release.)
    @return true if there is a viewport under the pointer
    */
    bool processMouseReleasedEvent(const MouseEvent& event);
    bool processMouseClickedEvent(const MouseEvent& event);
    bool processMouseDraggedEvent(const MouseEvent& event);

    /**
    Creates a copy of a mouse event, with its coordinates translated from the
    `ViewportSet` area to the given viewport (both with origin at the upper
    left corner).
    @return the translated event
    */
    MouseEvent toViewportEvent(const MouseEvent& event,
                               const Viewport* viewport) const;
};

#endif

#ifndef __TRANSLATE_GIZMO_INTERACTION_TECHNIQUE__
#define __TRANSLATE_GIZMO_INTERACTION_TECHNIQUE__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/gui/KeyEvent.h"
#include "vsdk/toolkit/gui/MouseEvent.h"

class TranslateGizmo;
class Viewport;

/**
Interaction technique to move a `TranslateGizmo` with the keyboard and the
mouse, processing only vitral events.

Mouse events must have coordinates in pixels of the viewport they are
processed for, with origin at its upper left corner.

Infinite drag: if the gesture is started with
`processMousePressedEvent(MouseEvent, Viewport)` over a handle of the gizmo,
the gesture belongs to that viewport until the button is released (see
`getDragViewport`), so the caller must keep on feeding it the events of the
gesture, whatever viewport the cursor is over. When the cursor leaves the
viewport, the technique keeps on working with "virtual" cursor coordinates
(the real ones plus the size of the viewport for each time the cursor wrapped
around) so the movement is continuous, and, if cursor wrapping is enabled,
requests to place the cursor at the opposite side of the viewport (see
`consumeCursorWarp`). Cursors are placed by the caller, as this technique
knows nothing about the GUI technology in use.

Numeric input: the technique also feeds the keyboard to the `InputGizmo` of
the gizmo, that shows (and lets the user type) its coordinates. Typing a
number and pressing ENTER moves the gizmo (so the caller, that applies its
movement to the things it manipulates, moves them precisely there); moving
the gizmo any other way discards what was typed.
*/
class TranslateGizmoInteractionTechnique {
public:
    /**
    Request to place the cursor at a position of the viewport where the
    gesture started, in pixels of that viewport (origin at its upper left
    corner).
    */
    class CursorWarp {
    private:
        int xValue;
        int yValue;

    public:
        CursorWarp() : xValue(0), yValue(0) {}
        CursorWarp(int x, int y) : xValue(x), yValue(y) {}
        int x() const { return xValue; }
        int y() const { return yValue; }
    };

private:
    TranslateGizmo* gizmo;

    Vector3Dd lastDeltaPosition;
    bool active;

    // State of the gesture in course (drag confined to a viewport)
    bool cursorWrapEnabled;
    Viewport* dragViewport;
    int dragSelection;
    bool wrappingSuspended;
    int wrapOffsetX;
    int wrapOffsetY;
    int lastVirtualX;
    int lastVirtualY;
    bool awaitingWarp;
    int warpTargetX;
    int warpTargetY;
    int staleEvents;
    bool hasPendingWarp;
    CursorWarp pendingWarp;

    void endGesture();
    bool checkWarpArrival(int x, int y);
    bool toVirtualEvent(const MouseEvent& e, MouseEvent* outVirtual);
    int calculateSelection(int x, int y);
    bool calculateInteractionPoint(const MouseEvent& e, Vector3Dd* outPoint);

public:
    /**
    @param gizmo gizmo manipulated by this technique (not owned)
    */
    explicit TranslateGizmoInteractionTechnique(TranslateGizmo* gizmo);
    virtual ~TranslateGizmoInteractionTechnique() {}

    TranslateGizmo* getGizmo() const;

    /**
    @return true if the input gizmo uses the key (digits, `-`, decimal point,
    TAB, BACKSPACE, and ENTER and ESC while editing), so the caller must
    not process it as any other command
    */
    bool isInputGizmoKey(const KeyEvent& keyEvent);

    /**
    @return true if the last cursor position was over an axis or plane handle
    of the gizmo
    */
    bool isActive() const;

    /**
    @return the viewport where the gesture in course started, or null if
    there is no gesture in course
    */
    Viewport* getDragViewport() const;

    /**
    Enables the wrapping of the cursor around the viewport while dragging. It
    should be enabled only if the caller is able to place the cursor when a
    warp is requested. It is disabled by default: the gesture is still
    confined to its viewport, but the cursor can leave it.
    */
    void setCursorWrapEnabled(bool cursorWrapEnabled);
    bool isCursorWrapEnabled() const;

    /**
    The caller should place the cursor as requested after processing each
    dragged event, and events already in course will be ignored by the
    technique until the cursor arrives.
    @param outWarp the pending request to place the cursor, if any
    @return true if there was a pending request; it is discarded after being
    returned (the Java version returns null when there is none)
    */
    bool consumeCursorWarp(CursorWarp* outWarp);

    bool processMouseEvent(const MouseEvent& mouseEvent);
    bool processKeyPressedEvent(const KeyEvent& keyEvent);
    bool processKeyReleasedEvent(const KeyEvent& keyEvent);

    /**
    Processes the press of a mouse button, starting a gesture confined to the
    given viewport if a handle of the gizmo is selected (that is, if dragging
    is going to move the gizmo).
    @return false (a press never changes the gizmo)
    */
    bool processMousePressedEvent(const MouseEvent& e, Viewport* viewport);

    /**
    Processes the press of a mouse button, without confining the gesture to
    any viewport.
    @return false (a press never changes the gizmo)
    */
    bool processMousePressedEvent(const MouseEvent& e);
    bool processMouseReleasedEvent(const MouseEvent& e);
    bool processMouseClickedEvent(const MouseEvent& e);
    bool processMouseMovedEvent(const MouseEvent& e);
    bool processMouseDraggedEvent(const MouseEvent& e);
    bool processMouseWheelEvent(const MouseEvent& e);
};

#endif

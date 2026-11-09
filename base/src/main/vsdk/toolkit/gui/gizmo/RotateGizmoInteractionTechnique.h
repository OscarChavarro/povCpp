#ifndef __ROTATE_GIZMO_INTERACTION_TECHNIQUE__
#define __ROTATE_GIZMO_INTERACTION_TECHNIQUE__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/gui/KeyEvent.h"
#include "vsdk/toolkit/gui/MouseEvent.h"

class RotateGizmo;
class Viewport;

/**
Interaction technique to work with a `RotateGizmo` using the keyboard and the
mouse, processing only vitral events.

Mouse events must have coordinates in pixels of the viewport the gizmo is
seen in, with origin at its upper left corner.

- Hover: the ring under the cursor is the volatile selection of the gizmo.
- Click: the ring under the cursor becomes the persistent selection of the
  gizmo; a click over no ring keeps the selection it has.
- Drag: pressing over a ring and dragging rotates the gizmo around the axis
  of the ring, so the point of the ring grabbed keeps under the cursor: the
  angle of the cursor around the axis is measured in the plane of the ring,
  and the gizmo takes the orientation it had when pressed plus that angle
  (so there is no drift, and turning several times is possible). While
  dragging the gizmo shows the arc swept (see `RotateGizmo::setArc`), which
  disappears on release. The gesture belongs to the viewport where it
  started, if it was given (see `getDragViewport`), so the caller must keep
  on feeding it the events of the gesture, whatever viewport the cursor is
  over. The angle is not updated while the plane of the ring is seen almost
  edge on, as the cursor does not define a point of it.
- Numeric input: the keyboard is fed to the `InputGizmo` of the gizmo, that
  shows (and lets the user type) the angles, in degrees, of its orientation.
  Typing numbers and pressing ENTER (or stepping them with the arrow keys)
  rotates the gizmo to exactly that orientation; rotating the gizmo any
  other way discards what was typed.
- The keys `x`, `y`, `z` (`X`, `Y`, `Z`) rotate the gizmo one degree
  clockwise (counterclockwise) around its own axes.
*/
class RotateGizmoInteractionTechnique {
private:
    RotateGizmo* gizmo;
    bool active;

    // State of the gesture in course (drag of a ring)
    int dragRing;
    Viewport* dragViewport;
    Matrix4x4d dragStartTransformation;
    Vector3Dd dragU;
    Vector3Dd dragV;
    Vector3Dd dragAxis;
    double dragStartAngle;
    double dragLastAngle;
    double dragSweep;

    void beginGesture(int ring, int x, int y);
    void endGesture();
    double calculateAngle(int x, int y);
    int calculateSelection(int x, int y);

public:
    /**
    @param gizmo gizmo manipulated by this technique (not owned)
    */
    explicit RotateGizmoInteractionTechnique(RotateGizmo* gizmo);
    virtual ~RotateGizmoInteractionTechnique() {}

    RotateGizmo* getGizmo() const;

    /**
    @return true if the last cursor position was over a ring of the gizmo
    */
    bool isActive() const;

    /**
    @return the viewport where the gesture in course started, or null if
    there is no gesture in course, or it was started without a viewport
    */
    Viewport* getDragViewport() const;

    /**
    @return true if a ring is being dragged
    */
    bool isDragging() const;

    /**
    @return true if the input gizmo uses the key (digits, `-`, decimal point,
    TAB, BACKSPACE, arrows, and ENTER and ESC while editing), so the caller
    must not process it as any other command
    */
    bool isInputGizmoKey(const KeyEvent& keyEvent);

    /**
    Processes a key press.
    @return true if the orientation of the gizmo changed
    */
    bool processKeyPressedEvent(const KeyEvent& keyEvent);
    bool processKeyReleasedEvent(const KeyEvent& keyEvent);

    /**
    Processes the press of a mouse button over the viewport where the gizmo
    is seen. If the cursor is over a ring, the ring becomes the chosen one
    and a gesture to rotate around it starts, confined to the given viewport.
    @return false (a press never changes the gizmo)
    */
    bool processMousePressedEvent(const MouseEvent& e, Viewport* viewport);

    /**
    Processes the press of a mouse button. If the cursor is over a ring, the
    ring becomes the chosen one and a gesture to rotate around it starts.
    @return false (a press never changes the gizmo)
    */
    bool processMousePressedEvent(const MouseEvent& e);

    /**
    Ends the gesture in course, if there is one, so the arc disappears.
    @return true if there was a gesture, so the gizmo must be drawn again
    */
    bool processMouseReleasedEvent(const MouseEvent& e);

    /**
    Makes the ring under the cursor the persistent selection, if there is
    one.
    @return true if the current selection changed
    */
    bool processMouseClickedEvent(const MouseEvent& e);

    /**
    Makes the ring under the cursor the volatile selection.
    @return true if the current selection changed
    */
    bool processMouseMovedEvent(const MouseEvent& e);

    /**
    Rotates the gizmo around the axis of the ring being dragged, so the point
    of the ring under the cursor when the gesture started follows it.
    @return true if the gizmo changed
    */
    bool processMouseDraggedEvent(const MouseEvent& e);
    bool processMouseWheelEvent(const MouseEvent& e);
};

#endif

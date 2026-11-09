#ifndef __SCALE_GIZMO_INTERACTION_TECHNIQUE__
#define __SCALE_GIZMO_INTERACTION_TECHNIQUE__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/gui/KeyEvent.h"
#include "vsdk/toolkit/gui/MouseEvent.h"

class ScaleGizmo;
class Viewport;

/**
Interaction technique to work with a `ScaleGizmo` using the keyboard and the
mouse, processing only vitral events, following the structure of
`RotateGizmoInteractionTechnique`.

Mouse events must have coordinates in pixels of the viewport the gizmo is
seen in, with origin at its upper left corner.

- Hover: the handle under the cursor (an axis, a two-axis band, or the
  uniform, all-axis triangle fan) is the volatile selection of the gizmo
  (see `ScaleGizmo::pickElement`).
- Click: the handle under the cursor becomes the persistent selection; a
  click over no handle keeps the selection it has.
- Drag: pressing over a handle and dragging scales every axis the handle's
  group includes (see `ScaleGizmo::groupIncludesAxis`) by the same factor:
  the ratio between the distance from the cursor to the projected origin of
  the gizmo while dragging, and that distance when the gesture started. The
  gesture belongs to the viewport where it started, if it was given (see
  `getDragViewport`), so the caller must keep on feeding it the events of the
  gesture, whatever viewport the cursor is over.
- Numeric input: the technique also feeds the keyboard to the `InputGizmo`
  of the gizmo, that shows (and lets the user type) its scale factors.
*/
class ScaleGizmoInteractionTechnique {
private:
    ScaleGizmo* gizmo;
    bool active;

    // State of the gesture in course (drag of a handle)
    Viewport* dragViewport;
    int dragGroup;
    bool hasDragStartScale;
    Vector3Dd dragStartScale;
    double dragStartDistance;

    double scaledIfIncluded(int axis, double ratio) const;
    void endGesture();
    int calculateSelection(int x, int y);
    double distanceToOrigin(int x, int y);

public:
    /**
    @param gizmo gizmo manipulated by this technique (not owned)
    */
    explicit ScaleGizmoInteractionTechnique(ScaleGizmo* gizmo);
    virtual ~ScaleGizmoInteractionTechnique() {}

    ScaleGizmo* getGizmo() const;
    bool isActive() const;

    /**
    @return the viewport where the gesture in course started, or null if
    there is no gesture in course, or it was started without a viewport
    */
    Viewport* getDragViewport() const;
    bool isInputGizmoKey(const KeyEvent& keyEvent);

    /**
    Feeds the key press to the gizmo (see
    `ScaleGizmo::processKeyPressedEvent`).
    @return true if the scale factors changed
    */
    bool processKeyPressedEvent(const KeyEvent& keyEvent);
    bool processKeyReleasedEvent(const KeyEvent& keyEvent);

    /**
    Processes the press of a mouse button, starting a gesture confined to the
    given viewport if a handle of the gizmo is selected.
    @return false (a press never changes the gizmo)
    */
    bool processMousePressedEvent(const MouseEvent& e, Viewport* viewport);

    /**
    Processes the press of a mouse button, without confining the gesture to
    any viewport. The gesture only begins if a handle is under the cursor.
    @return false (a press never changes the gizmo)
    */
    bool processMousePressedEvent(const MouseEvent& e);

    /**
    @return true if a handle of the gizmo is being dragged
    */
    bool isDragging() const;
    bool processMouseReleasedEvent(const MouseEvent& e);

    /**
    A click over a handle chooses it as the persistent selection; a click
    over no handle keeps the selection it has.
    @return true if the persistent selection changed
    */
    bool processMouseClickedEvent(const MouseEvent& e);

    /**
    While no gesture is in course, the handle under the cursor becomes the
    volatile selection.
    @return true if the volatile selection changed
    */
    bool processMouseMovedEvent(const MouseEvent& e);

    /**
    Scales every axis of the group being dragged by the ratio between the
    current and the starting distance from the cursor to the projected origin
    of the gizmo.
    @return true if the scale factors changed
    */
    bool processMouseDraggedEvent(const MouseEvent& e);
    bool processMouseWheelEvent(const MouseEvent& e);
};

#endif

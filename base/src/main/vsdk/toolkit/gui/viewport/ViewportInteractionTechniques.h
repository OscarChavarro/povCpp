#ifndef __VIEWPORT_INTERACTION_TECHNIQUES__
#define __VIEWPORT_INTERACTION_TECHNIQUES__

#include "vsdk/toolkit/gui/KeyEvent.h"
#include "vsdk/toolkit/gui/MouseEvent.h"
#include "vsdk/toolkit/gui/gizmo/TranslateGizmoInteractionTechnique.h"

class Camera;
class CameraController;
class InputGizmo;
class RendererConfiguration;
class RendererConfigurationController;
class RotateGizmo;
class RotateGizmoInteractionTechnique;
class ScaleGizmo;
class ScaleGizmoInteractionTechnique;
class TranslateGizmo;
class Viewport;

/**
Set of interaction techniques (camera controller, renderer configuration
controller and transformation gizmos with their techniques) used over the
viewports of a `ViewportSet`. It owns all of them; the camera and the
renderer configuration are referenced, not owned.
*/
class ViewportInteractionTechniques {
private:
    CameraController* cameraController;
    RendererConfigurationController* qualityController;
    TranslateGizmo* translationGizmo;
    TranslateGizmoInteractionTechnique* translationTechnique;
    RotateGizmo* rotateGizmo;
    RotateGizmoInteractionTechnique* rotationTechnique;
    ScaleGizmo* scaleGizmo;
    ScaleGizmoInteractionTechnique* scaleTechnique;

    ViewportInteractionTechniques(const ViewportInteractionTechniques& other);
    ViewportInteractionTechniques& operator=(
        const ViewportInteractionTechniques& other);

public:
    ViewportInteractionTechniques(Camera* camera,
                                  RendererConfiguration* rendererConfiguration);
    virtual ~ViewportInteractionTechniques();

    CameraController* getCameraController() const;
    RendererConfigurationController* getQualityController() const;
    TranslateGizmo* getTranslationGizmo() const;
    TranslateGizmoInteractionTechnique* getTranslationTechnique() const;
    RotateGizmo* getRotateGizmo() const;
    RotateGizmoInteractionTechnique* getRotationTechnique() const;
    ScaleGizmo* getScaleGizmo() const;
    void setCamera(Camera* camera);
    void setRendererConfiguration(RendererConfiguration* rendererConfiguration);

    bool processCameraKeyPressedEvent(const KeyEvent& event);
    bool processCameraKeyReleasedEvent(const KeyEvent& event);
    bool processCameraMousePressedEvent(const MouseEvent& event);
    bool processCameraMouseReleasedEvent(const MouseEvent& event);
    bool processCameraMouseClickedEvent(const MouseEvent& event);
    bool processCameraMouseMovedEvent(const MouseEvent& event);
    bool processCameraMouseDraggedEvent(const MouseEvent& event);
    bool processCameraMouseWheelEvent(const MouseEvent& event);
    bool processQualityKeyPressedEvent(const KeyEvent& event);

    /**
    @return the input gizmo that shows and edits the coordinates of the
    translation gizmo
    */
    InputGizmo* getTranslationInputGizmo();

    /**
    @return true if the input gizmo of the translation gizmo uses the key
    */
    bool isTranslationInputGizmoKey(const KeyEvent& event);
    bool processTranslationKeyPressedEvent(const KeyEvent& event);
    bool processTranslationMousePressedEvent(const MouseEvent& event);

    /**
    Processes the press of a mouse button over a viewport, starting a
    translation gesture confined to it (see `getTranslationDragViewport`).
    @return false (a press never changes the gizmo)
    */
    bool processTranslationMousePressedEvent(const MouseEvent& event,
                                             Viewport* viewport);

    /**
    @return the viewport where the translation gesture in course started, or
    null if there is none
    */
    Viewport* getTranslationDragViewport() const;

    /**
    @param enabled true if the caller is able to place the cursor when the
    translation technique requests it
    */
    void setTranslationCursorWrapEnabled(bool enabled);

    /**
    @param outWarp the pending request to place the cursor while dragging
    @return true if there was a pending request (the Java version returns
    null when there is none)
    */
    bool consumeTranslationCursorWarp(
        TranslateGizmoInteractionTechnique::CursorWarp* outWarp);
    bool processTranslationMouseReleasedEvent(const MouseEvent& event);
    bool processTranslationMouseClickedEvent(const MouseEvent& event);
    bool processTranslationMouseMovedEvent(const MouseEvent& event);
    bool processTranslationMouseDraggedEvent(const MouseEvent& event);

    /**
    @return the input gizmo that shows and edits the angles of the rotation
    gizmo
    */
    InputGizmo* getRotationInputGizmo();
    bool isRotationInputGizmoKey(const KeyEvent& event);
    bool processRotateKeyPressedEvent(const KeyEvent& event);
    bool processRotationMousePressedEvent(const MouseEvent& event);

    /**
    Processes the press of a mouse button over a viewport, starting a
    rotation gesture confined to it if the pointer is over a ring.
    @return false (a press never changes the gizmo)
    */
    bool processRotationMousePressedEvent(const MouseEvent& event,
                                          Viewport* viewport);
    Viewport* getRotationDragViewport() const;
    bool processRotationMouseReleasedEvent(const MouseEvent& event);
    bool processRotationMouseDraggedEvent(const MouseEvent& event);
    bool processRotationMouseClickedEvent(const MouseEvent& event);
    bool processRotationMouseMovedEvent(const MouseEvent& event);
    bool processScaleKeyPressedEvent(const KeyEvent& event);

    /**
    @return the input gizmo that shows and edits the scale factors of the
    scale gizmo
    */
    InputGizmo* getScaleInputGizmo();
    bool isScaleInputGizmoKey(const KeyEvent& event);

    /**
    @return the interaction technique that hovers, selects and drags the
    handles of the scale gizmo
    */
    ScaleGizmoInteractionTechnique* getScaleTechnique() const;
    bool processScaleMousePressedEvent(const MouseEvent& event);

    /**
    Processes the press of a mouse button over a viewport, starting a scale
    gesture confined to it if the pointer is over a handle.
    @return false (a press never changes the gizmo)
    */
    bool processScaleMousePressedEvent(const MouseEvent& event,
                                       Viewport* viewport);
    Viewport* getScaleDragViewport() const;
    bool processScaleMouseReleasedEvent(const MouseEvent& event);
    bool processScaleMouseDraggedEvent(const MouseEvent& event);
    bool processScaleMouseClickedEvent(const MouseEvent& event);
    bool processScaleMouseMovedEvent(const MouseEvent& event);
};

#endif

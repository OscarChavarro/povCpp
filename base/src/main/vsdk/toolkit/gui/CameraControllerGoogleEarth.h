#ifndef __CAMERA_CONTROLLER_GOOGLE_EARTH__
#define __CAMERA_CONTROLLER_GOOGLE_EARTH__

#include "vsdk/toolkit/gui/CameraController.h"
class Camera;

class CameraControllerGoogleEarth : public CameraController {
public:
    explicit CameraControllerGoogleEarth(Camera* camera);
    virtual ~CameraControllerGoogleEarth() {}

    bool processKeyPressedEvent(const KeyEvent& keyEvent) override;
    bool processKeyReleasedEvent(const KeyEvent& keyEvent) override;
    bool processMousePressedEvent(const MouseEvent& e) override;
    bool processMouseReleasedEvent(const MouseEvent& e) override;
    bool processMouseClickedEvent(const MouseEvent& e) override;
    bool processMouseMovedEvent(const MouseEvent& e) override;
    bool processMouseDraggedEvent(const MouseEvent& e) override;
    bool processMouseWheelEvent(const MouseEvent& e) override;

    Camera* getCamera() override;
    void setCamera(Camera* camera) override;
    void setDeltaMovement(double factor) override;
    void tick(double inCurrentTime) override {}

    void zoomOut(double jumpValue);
    void zoomIn(double jumpValue);

private:
    Camera* camera;
    double jumpStep;
    int xOld;
    int yOld;
};

#endif

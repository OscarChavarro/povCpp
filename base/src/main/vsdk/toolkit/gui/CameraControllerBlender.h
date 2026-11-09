#ifndef __CAMERA_CONTROLLER_BLENDER__
#define __CAMERA_CONTROLLER_BLENDER__

#include "vsdk/toolkit/gui/CameraController.h"
class Camera;

class CameraControllerBlender : public CameraController {
public:
    explicit CameraControllerBlender(Camera* camera);
    virtual ~CameraControllerBlender() {}

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

private:
    Camera* camera;

    double augmentLogarithmic(double val, double epsilon);
    double diminishLogarithmic(double val, double epsilon);
};

#endif

#ifndef __CAMERA_CONTROLLER_AQUYNZA__
#define __CAMERA_CONTROLLER_AQUYNZA__

#include "vsdk/toolkit/gui/CameraController.h"
class Camera;

class CameraControllerAquynza : public CameraController {
public:
    explicit CameraControllerAquynza(Camera* camera);
    virtual ~CameraControllerAquynza() {}

    double getDeltaMovement() const;
    void setDeltaMovement(double val) override;

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
    void tick(double inCurrentTime) override {}

private:
    Camera* camera;
    int oldMouseX;
    int oldMouseY;
    double deltaMovement;

    double augmentLogarithmic(double val, double EPSILON);
    double diminishLogarithmic(double val, double EPSILON);
};


#endif

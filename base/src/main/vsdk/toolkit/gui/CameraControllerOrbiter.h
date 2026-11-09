#ifndef __CAMERA_CONTROLLER_ORBITER__
#define __CAMERA_CONTROLLER_ORBITER__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/gui/CameraController.h"
class Camera;

class CameraControllerOrbiter : public CameraController {
public:
    explicit CameraControllerOrbiter(Camera* camera);
    virtual ~CameraControllerOrbiter() {}

    double getDeltaMovement() const;
    void setDeltaMovement(double val) override;

    Vector3Dd getPointOfInterest() const;
    void setPointOfInterest(const Vector3Dd& poi);

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
    Vector3Dd pointOfInterest;

    double augmentLogarithmic(double val, double epsilon);
    double diminishLogarithmic(double val, double epsilon);
    bool orbitAroundPointOfInterest(double yawDelta, double pitchDelta);
};

#endif

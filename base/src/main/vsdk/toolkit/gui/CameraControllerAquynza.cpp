#include <cmath>
#include <cstdio>

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/camera/Camera.h"
#include "vsdk/toolkit/gui/CameraControllerAquynza.h"
static const double PI = 3.14159265358979323846;

static inline double degreesToRadians(double degrees) {
    return degrees * PI / 180.0;
}

static inline double radiansToDegrees(double radians) {
    return radians * 180.0 / PI;
}

CameraControllerAquynza::CameraControllerAquynza(Camera* cam)
    : camera(cam), oldMouseX(0), oldMouseY(0), deltaMovement(0.25) {
}

double CameraControllerAquynza::getDeltaMovement() const {
    return deltaMovement;
}

void CameraControllerAquynza::setDeltaMovement(double val) {
    deltaMovement = val;
}

double CameraControllerAquynza::augmentLogarithmic(double val, double EPSILON) {
    if (val < 0.001) val += 0.0001;
    else if (val < 0.01) val += 0.001;
    else if (val < 0.1 - EPSILON) val += 0.01;
    else if (val < 1 - EPSILON) val += 0.1;
    else if (val < 10 - EPSILON) val += 1;
    else if (val < 100 - EPSILON) val += 10;
    else if (val < 1000 - EPSILON) val += 100;
    else if (val < 10000 - EPSILON) val += 1000;
    else if (val < 100000 - EPSILON) val += 10000;
    else if (val < 1000000 - EPSILON) val += 100000;
    else if (val < 10000000 - EPSILON) val *= 2;
    else val = 10000000;
    return val;
}

double CameraControllerAquynza::diminishLogarithmic(double val, double EPSILON) {
    if (val > 10000000 + EPSILON) val /= 2;
    else if (val > 1000000 + EPSILON) val -= 1000000;
    else if (val > 100000 + EPSILON) val -= 100000;
    else if (val > 10000 + EPSILON) val -= 10000;
    else if (val > 1000 + EPSILON) val -= 1000;
    else if (val > 100 + EPSILON) val -= 100;
    else if (val > 10 + EPSILON) val -= 10;
    else if (val > 1 + EPSILON) val -= 1;
    else if (val > 0.1 + EPSILON) val -= 0.1;
    else if (val > 0.01 + EPSILON) val -= 0.01;
    else if (val > 0.001 + EPSILON) val -= 0.001;
    else if (val > 0.0001 + EPSILON) val -= 0.0001;
    else val = 0.0001;
    return val;
}

bool CameraControllerAquynza::processKeyPressedEvent(const KeyEvent& keyEvent) {
    Vector3Dd eyePosition = camera->getPosition();
    Vector3Dd focusedPosition = camera->getFocusedPosition();
    Matrix4x4d R = camera->getRotation();
    int projectionMode = camera->getProjectionMode();
    double fov = camera->getFov();
    double orthogonalZoom = camera->getOrthogonalZoom();
    double nearPlaneDistance = camera->getNearPlaneDistance();
    double farPlaneDistance = camera->getFarPlaneDistance();

    double yaw = R.obtainEulerYawAngle();
    double pitch = R.obtainEulerPitchAngle();
    double roll = R.obtainEulerRollAngle();

    double angleInc;
    bool updated = false;
    const double EPSILON = 0.0001;

    if (fov > 90) angleInc = degreesToRadians(10);
    else if (fov > 45) angleInc = degreesToRadians(5);
    else if (fov > 15) angleInc = degreesToRadians(2.5);
    else if (fov > 5) angleInc = degreesToRadians(1);
    else angleInc = degreesToRadians(0.1);

    switch (keyEvent.keycode) {
        case KeyEvent::KEY_UP:
            pitch -= angleInc;
            if (pitch < degreesToRadians(-90)) pitch = degreesToRadians(-90);
            updated = true;
            break;
        case KeyEvent::KEY_DOWN:
            pitch += angleInc;
            if (pitch > degreesToRadians(90)) pitch = degreesToRadians(90);
            updated = true;
            break;
        case KeyEvent::KEY_LEFT:
            yaw += angleInc;
            while (yaw >= degreesToRadians(360)) yaw -= degreesToRadians(360);
            updated = true;
            break;
        case KeyEvent::KEY_RIGHT:
            yaw -= angleInc;
            while (yaw < 0) yaw += degreesToRadians(360);
            updated = true;
            break;

        case KeyEvent::KEY_x:
            eyePosition = eyePosition.withX(eyePosition.x() - deltaMovement);
            focusedPosition = focusedPosition.withX(focusedPosition.x() - deltaMovement);
            updated = true;
            break;
        case KeyEvent::KEY_X:
            eyePosition = eyePosition.withX(eyePosition.x() + deltaMovement);
            focusedPosition = focusedPosition.withX(focusedPosition.x() + deltaMovement);
            updated = true;
            break;
        case KeyEvent::KEY_y:
            eyePosition = eyePosition.withY(eyePosition.y() - deltaMovement);
            focusedPosition = focusedPosition.withY(focusedPosition.y() - deltaMovement);
            updated = true;
            break;
        case KeyEvent::KEY_Y:
            eyePosition = eyePosition.withY(eyePosition.y() + deltaMovement);
            focusedPosition = focusedPosition.withY(focusedPosition.y() + deltaMovement);
            updated = true;
            break;
        case KeyEvent::KEY_z:
            eyePosition = eyePosition.withZ(eyePosition.z() - deltaMovement);
            focusedPosition = focusedPosition.withZ(focusedPosition.z() - deltaMovement);
            updated = true;
            break;
        case KeyEvent::KEY_Z:
            eyePosition = eyePosition.withZ(eyePosition.z() + deltaMovement);
            focusedPosition = focusedPosition.withZ(focusedPosition.z() + deltaMovement);
            updated = true;
            break;

        case KeyEvent::KEY_S:
            roll -= degreesToRadians(5);
            while (roll < 0) roll += degreesToRadians(360);
            updated = true;
            break;
        case KeyEvent::KEY_s:
            roll += degreesToRadians(5);
            while (roll > degreesToRadians(360)) roll -= degreesToRadians(360);
            updated = true;
            break;

        case KeyEvent::KEY_A:
            if (camera->getProjectionMode() == Camera::PROJECTION_MODE_ORTHOGONAL) {
                orthogonalZoom /= 2;
            } else {
                if (fov < 0.1 - EPSILON) fov += 0.1;
                else if (fov < 1 - EPSILON) fov++;
                else if (fov < 175 - EPSILON) fov += 5;
            }
            updated = true;
            break;
        case KeyEvent::KEY_a:
            if (camera->getProjectionMode() == Camera::PROJECTION_MODE_ORTHOGONAL) {
                orthogonalZoom *= 2;
            } else {
                if (fov > 5 + EPSILON) fov -= 5;
                else if (fov > 1 + EPSILON) fov--;
                else if (fov > 0.1 + EPSILON) fov -= 0.1;
            }
            updated = true;
            break;

        case KeyEvent::KEY_N:
            nearPlaneDistance = augmentLogarithmic(nearPlaneDistance, EPSILON);
            updated = true;
            break;
        case KeyEvent::KEY_n:
            nearPlaneDistance = diminishLogarithmic(nearPlaneDistance, EPSILON);
            updated = true;
            break;

        case KeyEvent::KEY_F:
            farPlaneDistance = augmentLogarithmic(farPlaneDistance, EPSILON);
            updated = true;
            break;
        case KeyEvent::KEY_f:
            farPlaneDistance = diminishLogarithmic(farPlaneDistance, EPSILON);
            updated = true;
            break;

        case KeyEvent::KEY_p:
            switch (projectionMode) {
                case Camera::PROJECTION_MODE_PERSPECTIVE:
                    projectionMode = Camera::PROJECTION_MODE_ORTHOGONAL;
                    break;
                default:
                    projectionMode = Camera::PROJECTION_MODE_PERSPECTIVE;
                    break;
            }
            updated = true;
            break;

        case KeyEvent::KEY_i:
            fprintf(stdout, "%s\n", camera->toString().c_str());
            break;
    }

    R = R.eulerAnglesRotation(yaw, pitch, roll);

    camera->setPosition(eyePosition);
    camera->setFocusedPositionMaintainingOrthogonality(focusedPosition);
    camera->setRotation(R);
    camera->setOrthogonalZoom(orthogonalZoom);
    camera->setFov(fov);
    camera->setProjectionMode(projectionMode);
    camera->setNearPlaneDistance(nearPlaneDistance);
    camera->setFarPlaneDistance(farPlaneDistance);

    return updated;
}

bool CameraControllerAquynza::processKeyReleasedEvent(const KeyEvent& keyEvent) {
    return false;
}

bool CameraControllerAquynza::processMousePressedEvent(const MouseEvent& e) {
    oldMouseX = e.getX();
    oldMouseY = e.getY();
    return false;
}

bool CameraControllerAquynza::processMouseReleasedEvent(const MouseEvent& e) {
    return false;
}

bool CameraControllerAquynza::processMouseClickedEvent(const MouseEvent& e) {
    return false;
}

bool CameraControllerAquynza::processMouseMovedEvent(const MouseEvent& e) {
    return false;
}

bool CameraControllerAquynza::processMouseDraggedEvent(const MouseEvent& e) {
    int deltaX = e.getX() - oldMouseX;
    int deltaY = e.getY() - oldMouseY;
    bool updated = false;
    double senseFactor = deltaMovement / 5.0;

    deltaX = java::Math::max(-5, java::Math::min(5, deltaX));
    deltaY = java::Math::max(-5, java::Math::min(5, deltaY));

    Matrix4x4d R = camera->getRotation();
    Vector3Dd u(R.get(0, 0), R.get(1, 0), R.get(2, 0));
    Vector3Dd v(R.get(0, 1), R.get(1, 1), R.get(2, 1));
    Vector3Dd w(R.get(0, 2), R.get(1, 2), R.get(2, 2));

    Vector3Dd eyePosition = camera->getPosition();
    Vector3Dd focusedPosition = camera->getFocusedPosition();

    int modifiers = e.getModifiers();

    if ((modifiers & MouseEvent::BUTTON1_DOWN_MASK) != 0) {
        double ax = -java::Math::min(2.0, 0.01 * deltaX);
        double ay = java::Math::min(2.0, 0.01 * deltaY);

        Matrix4x4d DR = Matrix4x4d().axisRotation(ay, v.x(), v.y(), v.z());
        R = DR.multiply(R);

        DR = Matrix4x4d().axisRotation(ax, w.x(), w.y(), w.z());
        R = DR.multiply(R);

        updated = true;
    } else if ((modifiers & MouseEvent::BUTTON2_DOWN_MASK) != 0) {
        eyePosition = eyePosition.subtract(v.multiply(senseFactor * deltaX));
        eyePosition = eyePosition.subtract(w.multiply(senseFactor * deltaY));
        focusedPosition = focusedPosition.subtract(v.multiply(senseFactor * deltaX));
        focusedPosition = focusedPosition.subtract(w.multiply(senseFactor * deltaY));
        updated = true;
    } else if ((modifiers & MouseEvent::BUTTON3_DOWN_MASK) != 0) {
        eyePosition = eyePosition.subtract(u.multiply(senseFactor * deltaY));
        double ax = java::Math::min(2.0, 0.01 * deltaX);
        Matrix4x4d DR = Matrix4x4d().axisRotation(ax, u.x(), u.y(), u.z());
        R = DR.multiply(R);
        updated = true;
    }

    camera->setPosition(eyePosition);
    camera->setFocusedPositionMaintainingOrthogonality(focusedPosition);
    camera->setRotation(R);

    oldMouseX = e.getX();
    oldMouseY = e.getY();
    return updated;
}

bool CameraControllerAquynza::processMouseWheelEvent(const MouseEvent& e) {
    double fov = camera->getFov();
    double orthogonalZoom = camera->getOrthogonalZoom();
    const double EPSILON = 0.0001;

    double angleInc;
    if (fov > 90) angleInc = degreesToRadians(10);
    else if (fov > 45) angleInc = degreesToRadians(5);
    else if (fov > 15) angleInc = degreesToRadians(2.5);
    else if (fov > 5) angleInc = degreesToRadians(1);
    else angleInc = degreesToRadians(0.1);

    int clicks = e.getClicks();

    if (clicks > 0) {
        if (camera->getProjectionMode() == Camera::PROJECTION_MODE_ORTHOGONAL) {
            orthogonalZoom /= (2 * clicks);
        } else {
            if (fov < 0.1 - EPSILON) fov += 0.1 * clicks;
            else if (fov < 1 - EPSILON) fov += clicks;
            else if (fov < 175 - EPSILON) fov += 5 * clicks;
        }
    } else if (clicks < 0) {
        if (camera->getProjectionMode() == Camera::PROJECTION_MODE_ORTHOGONAL) {
            orthogonalZoom *= (2 * -clicks);
        } else {
            if (fov > 5 + EPSILON) fov += 5 * clicks;
            else if (fov > 1 + EPSILON) fov += clicks;
            else if (fov > 0.1 + EPSILON) fov += 0.1 * clicks;
        }
    }

    camera->setFov(fov);
    camera->setOrthogonalZoom(orthogonalZoom);

    return clicks != 0;
}

Camera* CameraControllerAquynza::getCamera() {
    return camera;
}

void CameraControllerAquynza::setCamera(Camera* cam) {
    camera = cam;
}

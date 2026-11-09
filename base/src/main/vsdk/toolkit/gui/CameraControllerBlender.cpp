#include <cmath>
#include <cstdio>

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/camera/Camera.h"
#include "vsdk/toolkit/gui/CameraControllerBlender.h"
static const double PI = 3.14159265358979323846;

CameraControllerBlender::CameraControllerBlender(Camera* camera)
    : camera(camera)
{
}

double CameraControllerBlender::augmentLogarithmic(double val, double epsilon)
{
    if ( val < 0.001 ) val += 0.0001;
    else if ( val < 0.01 ) val += 0.001;
    else if ( val < 0.1 - epsilon ) val += 0.01;
    else if ( val < 1 - epsilon ) val += 0.1;
    else if ( val < 10 - epsilon ) val += 1;
    else if ( val < 100 - epsilon ) val += 10;
    else if ( val < 1000 - epsilon ) val += 100;
    else if ( val < 10000 - epsilon ) val += 1000;
    else if ( val < 100000 - epsilon ) val += 10000;
    else if ( val < 1000000 - epsilon ) val += 100000;
    else if ( val < 10000000 - epsilon ) val *= 2;
    else val = 10000000;

    return val;
}

double CameraControllerBlender::diminishLogarithmic(double val, double epsilon)
{
    if ( val > 10000000 + epsilon ) val /= 2;
    else if ( val > 1000000 + epsilon ) val -= 1000000;
    else if ( val > 100000 + epsilon ) val -= 100000;
    else if ( val > 10000 + epsilon ) val -= 10000;
    else if ( val > 1000 + epsilon ) val -= 1000;
    else if ( val > 100 + epsilon ) val -= 100;
    else if ( val > 10 + epsilon ) val -= 10;
    else if ( val > 1 + epsilon ) val -= 1;
    else if ( val > 0.1 + epsilon ) val -= 0.1;
    else if ( val > 0.01 + epsilon ) val -= 0.01;
    else if ( val > 0.001 + epsilon ) val -= 0.001;
    else if ( val > 0.0001 + epsilon ) val -= 0.0001;
    else val = 0.0001;
    return val;
}

bool CameraControllerBlender::processKeyPressedEvent(const KeyEvent& keyEvent)
{
    Vector3Dd eyePosition = camera->getPosition();
    Vector3Dd focusedPosition = camera->getFocusedPosition();
    Matrix4x4d R = camera->getRotation();
    int projectionMode = camera->getProjectionMode();
    double fov = camera->getFov();
    double orthogonalZoom = camera->getOrthogonalZoom();
    double nearPlaneDistance = camera->getNearPlaneDistance();
    double farPlaneDistance = camera->getFarPlaneDistance();

    const double deltaMov = 100.0;
    bool updated = false;
    const int shiftMask = KeyEvent::MASK_SHIFT;
    const int ctrlMask = KeyEvent::MASK_CTRL;

    int mask = keyEvent.modifierMask;
    Vector3Dd v(R.get(0, 1), R.get(1, 1), R.get(2, 1));
    Vector3Dd w(R.get(0, 2), R.get(1, 2), R.get(2, 2));
    Vector3Dd u(R.get(0, 0), R.get(1, 0), R.get(2, 0));

    Matrix4x4d tranMat;
    double angRot = PI * 5.0 / 180.0;

    switch ( keyEvent.keycode ) {
      case KeyEvent::KEY_X:
        if ( (mask & shiftMask) == 0 ) {
            eyePosition = eyePosition.withX(eyePosition.x() - deltaMov);
            focusedPosition = focusedPosition.withX(focusedPosition.x() - deltaMov);
        }
        else {
            eyePosition = eyePosition.withX(eyePosition.x() + deltaMov);
            focusedPosition = focusedPosition.withX(focusedPosition.x() + deltaMov);
        }
        updated = true;
        break;

      case KeyEvent::KEY_NUM5:
        projectionMode = (projectionMode == Camera::PROJECTION_MODE_PERSPECTIVE)
            ? Camera::PROJECTION_MODE_ORTHOGONAL
            : Camera::PROJECTION_MODE_PERSPECTIVE;
        updated = true;
        break;

      case KeyEvent::KEY_NUM4:
        if ( (mask & ctrlMask) == 0 ) {
            tranMat = tranMat.axisRotation(-angRot, 0, 0, 1);
            R = tranMat.multiply(R);
            eyePosition = tranMat.multiply(eyePosition);
        }
        else {
            eyePosition = eyePosition.add(v.multiply(deltaMov));
            focusedPosition = focusedPosition.add(v.multiply(deltaMov));
        }
        updated = true;
        break;

      case KeyEvent::KEY_NUM6:
        if ( (mask & ctrlMask) == 0 ) {
            tranMat = tranMat.axisRotation(angRot, 0, 0, 1);
            R = tranMat.multiply(R);
            eyePosition = tranMat.multiply(eyePosition);
        }
        else {
            eyePosition = eyePosition.subtract(v.multiply(deltaMov));
            focusedPosition = focusedPosition.subtract(v.multiply(deltaMov));
        }
        updated = true;
        break;

      case KeyEvent::KEY_NUM2:
        if ( (mask & ctrlMask) == 0 ) {
            tranMat = tranMat.axisRotation(-angRot, v);
            R = tranMat.multiply(R);
            eyePosition = tranMat.multiply(eyePosition);
        }
        else {
            eyePosition = eyePosition.subtract(w.multiply(deltaMov));
            focusedPosition = focusedPosition.subtract(w.multiply(deltaMov));
        }
        updated = true;
        break;

      case KeyEvent::KEY_NUM8:
        if ( (mask & ctrlMask) == 0 ) {
            tranMat = tranMat.axisRotation(angRot, v);
            R = tranMat.multiply(R);
            eyePosition = tranMat.multiply(eyePosition);
        }
        else {
            eyePosition = eyePosition.add(w.multiply(deltaMov));
            focusedPosition = focusedPosition.add(w.multiply(deltaMov));
        }
        updated = true;
        break;

      case KeyEvent::KEY_NUM1:
        R = R.axisRotation(0, 0, 0, 1);
        updated = true;
        break;

      case KeyEvent::KEY_NUMPLUS:
        eyePosition = eyePosition.add(u.multiply(deltaMov));
        focusedPosition = focusedPosition.add(u.multiply(deltaMov));
        updated = true;
        break;

      case KeyEvent::KEY_NUMMINUS:
        eyePosition = eyePosition.subtract(u.multiply(deltaMov));
        focusedPosition = focusedPosition.subtract(u.multiply(deltaMov));
        updated = true;
        break;

      case KeyEvent::KEY_N:
        nearPlaneDistance = augmentLogarithmic(nearPlaneDistance, 0.0001);
        updated = true;
        break;
      case KeyEvent::KEY_n:
        nearPlaneDistance = diminishLogarithmic(nearPlaneDistance, 0.0001);
        updated = true;
        break;
      case KeyEvent::KEY_F:
        farPlaneDistance = augmentLogarithmic(farPlaneDistance, 0.0001);
        updated = true;
        break;
      case KeyEvent::KEY_f:
        farPlaneDistance = diminishLogarithmic(farPlaneDistance, 0.0001);
        updated = true;
        break;

      case KeyEvent::KEY_i:
        fprintf(stdout, "%s\n", camera->toString().c_str());
        break;
    }

    double d = eyePosition.length();
    if ( d < 0.1 ) orthogonalZoom = 10;
    else orthogonalZoom = 1 / d;

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

bool CameraControllerBlender::processKeyReleasedEvent(const KeyEvent&)
{
    return false;
}

bool CameraControllerBlender::processMousePressedEvent(const MouseEvent&)
{
    return false;
}

bool CameraControllerBlender::processMouseReleasedEvent(const MouseEvent&)
{
    return false;
}

bool CameraControllerBlender::processMouseClickedEvent(const MouseEvent&)
{
    return false;
}

bool CameraControllerBlender::processMouseMovedEvent(const MouseEvent&)
{
    return false;
}

bool CameraControllerBlender::processMouseDraggedEvent(const MouseEvent&)
{
    return false;
}

bool CameraControllerBlender::processMouseWheelEvent(const MouseEvent&)
{
    return false;
}

Camera* CameraControllerBlender::getCamera()
{
    return camera;
}

void CameraControllerBlender::setCamera(Camera* camera)
{
    this->camera = camera;
}

void CameraControllerBlender::setDeltaMovement(double)
{
}

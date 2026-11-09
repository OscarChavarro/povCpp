#include <cmath>
#include <cstdio>

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/camera/Camera.h"
#include "vsdk/toolkit/gui/CameraControllerOrbiter.h"
static const double PI = 3.14159265358979323846;

CameraControllerOrbiter::CameraControllerOrbiter(Camera* cam)
    : camera(cam), oldMouseX(0), oldMouseY(0), deltaMovement(0.25),
      pointOfInterest(0.0, 0.0, 0.0)
{
}

double CameraControllerOrbiter::getDeltaMovement() const
{
    return deltaMovement;
}

void CameraControllerOrbiter::setDeltaMovement(double val)
{
    deltaMovement = val;
}

Vector3Dd CameraControllerOrbiter::getPointOfInterest() const
{
    return Vector3Dd::copyOf(pointOfInterest);
}

void CameraControllerOrbiter::setPointOfInterest(const Vector3Dd& poi)
{
    pointOfInterest = Vector3Dd::copyOf(poi);
}

double CameraControllerOrbiter::augmentLogarithmic(double val, double epsilon)
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

double CameraControllerOrbiter::diminishLogarithmic(double val, double epsilon)
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

bool CameraControllerOrbiter::orbitAroundPointOfInterest(double yawDelta,
                                                         double pitchDelta)
{
    Vector3Dd eyePosition = camera->getPosition();
    Vector3Dd offset = eyePosition.subtract(pointOfInterest);

    if ( offset.length() < 1e-9 ) {
        return false;
    }

    Matrix4x4d R = camera->getRotation();
    Matrix4x4d DR;
    Vector3Dd axisLeft(R.get(0, 1), R.get(1, 1), R.get(2, 1));
    Vector3Dd axisUp(R.get(0, 2), R.get(1, 2), R.get(2, 2));
    axisLeft = axisLeft.normalized();
    axisUp = axisUp.normalized();

    if ( std::abs(pitchDelta) > 0 ) {
        DR = Matrix4x4d().axisRotation(
            pitchDelta,
            axisLeft.x(), axisLeft.y(), axisLeft.z());
        offset = DR.multiply(offset);
    }
    if ( std::abs(yawDelta) > 0 ) {
        DR = Matrix4x4d().axisRotation(
            yawDelta,
            axisUp.x(), axisUp.y(), axisUp.z());
        offset = DR.multiply(offset);
    }

    Vector3Dd newEyePosition = pointOfInterest.add(offset);
    Vector3Dd newFrontDirection = pointOfInterest.subtract(newEyePosition);
    newFrontDirection = newFrontDirection.normalized();
    Vector3Dd upHint = camera->getUp();
    if ( std::abs(newFrontDirection.dotProduct(upHint)) > 0.99999 ) {
        return false;
    }

    camera->setPosition(newEyePosition);
    camera->setFocusedPositionMaintainingOrthogonality(pointOfInterest);
    return true;
}

bool CameraControllerOrbiter::processKeyPressedEvent(const KeyEvent& keyEvent)
{
    Vector3Dd eyePosition = camera->getPosition();
    Vector3Dd focusedPosition = camera->getFocusedPosition();
    Matrix4x4d R = camera->getRotation();
    int projectionMode = camera->getProjectionMode();
    double fov = camera->getFov();
    double orthogonalZoom = camera->getOrthogonalZoom();
    double nearPlaneDistance = camera->getNearPlaneDistance();
    double farPlaneDistance = camera->getFarPlaneDistance();

    double angleInc;
    bool updated = false;
    const double epsilon = 0.0001;
    double roll = R.obtainEulerRollAngle();

    if ( fov > 90 ) angleInc = PI * 10.0 / 180.0;
    else if ( fov > 45 ) angleInc = PI * 5.0 / 180.0;
    else if ( fov > 15 ) angleInc = PI * 2.5 / 180.0;
    else if ( fov > 5 ) angleInc = PI * 1.0 / 180.0;
    else angleInc = PI * 0.1 / 180.0;

    switch ( keyEvent.keycode ) {
      case KeyEvent::KEY_UP:
        updated = orbitAroundPointOfInterest(0, -angleInc);
        break;
      case KeyEvent::KEY_DOWN:
        updated = orbitAroundPointOfInterest(0, angleInc);
        break;
      case KeyEvent::KEY_LEFT:
        updated = orbitAroundPointOfInterest(angleInc, 0);
        break;
      case KeyEvent::KEY_RIGHT:
        updated = orbitAroundPointOfInterest(-angleInc, 0);
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
        roll -= PI * 5.0 / 180.0;
        while ( roll < 0 ) roll += PI * 2.0;
        R = R.eulerAnglesRotation(
            R.obtainEulerYawAngle(),
            R.obtainEulerPitchAngle(),
            roll);
        updated = true;
        break;
      case KeyEvent::KEY_s:
        roll += PI * 5.0 / 180.0;
        while ( roll > PI * 2.0 ) roll -= PI * 2.0;
        R = R.eulerAnglesRotation(
            R.obtainEulerYawAngle(),
            R.obtainEulerPitchAngle(),
            roll);
        updated = true;
        break;

      case KeyEvent::KEY_A:
        if ( camera->getProjectionMode() == Camera::PROJECTION_MODE_ORTHOGONAL ) {
            orthogonalZoom /= 2;
        }
        else {
            if ( fov < 0.1 - epsilon ) fov += 0.1;
            else if ( fov < 1 - epsilon ) fov++;
            else if ( fov < 175 - epsilon ) fov += 5;
        }
        updated = true;
        break;
      case KeyEvent::KEY_a:
        if ( camera->getProjectionMode() == Camera::PROJECTION_MODE_ORTHOGONAL ) {
            orthogonalZoom *= 2;
        }
        else {
            if ( fov > 5 + epsilon ) fov -= 5;
            else if ( fov > 1 + epsilon ) fov--;
            else if ( fov > 0.1 + epsilon ) fov -= 0.1;
        }
        updated = true;
        break;

      case KeyEvent::KEY_N:
        nearPlaneDistance = augmentLogarithmic(nearPlaneDistance, epsilon);
        updated = true;
        break;
      case KeyEvent::KEY_n:
        nearPlaneDistance = diminishLogarithmic(nearPlaneDistance, epsilon);
        updated = true;
        break;

      case KeyEvent::KEY_F:
        farPlaneDistance = augmentLogarithmic(farPlaneDistance, epsilon);
        updated = true;
        break;
      case KeyEvent::KEY_f:
        farPlaneDistance = diminishLogarithmic(farPlaneDistance, epsilon);
        updated = true;
        break;

      case KeyEvent::KEY_p:
        switch ( projectionMode ) {
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

    if ( keyEvent.keycode != KeyEvent::KEY_UP &&
         keyEvent.keycode != KeyEvent::KEY_DOWN &&
         keyEvent.keycode != KeyEvent::KEY_LEFT &&
         keyEvent.keycode != KeyEvent::KEY_RIGHT ) {
        camera->setPosition(eyePosition);
        camera->setFocusedPositionMaintainingOrthogonality(focusedPosition);
        camera->setRotation(R);
    }
    camera->setOrthogonalZoom(orthogonalZoom);
    camera->setFov(fov);
    camera->setProjectionMode(projectionMode);
    camera->setNearPlaneDistance(nearPlaneDistance);
    camera->setFarPlaneDistance(farPlaneDistance);

    return updated;
}

bool CameraControllerOrbiter::processKeyReleasedEvent(const KeyEvent&)
{
    return false;
}

bool CameraControllerOrbiter::processMousePressedEvent(const MouseEvent& e)
{
    oldMouseX = e.getX();
    oldMouseY = e.getY();
    return false;
}

bool CameraControllerOrbiter::processMouseReleasedEvent(const MouseEvent&)
{
    return false;
}

bool CameraControllerOrbiter::processMouseClickedEvent(const MouseEvent&)
{
    return false;
}

bool CameraControllerOrbiter::processMouseMovedEvent(const MouseEvent&)
{
    return false;
}

bool CameraControllerOrbiter::processMouseDraggedEvent(const MouseEvent& e)
{
    int deltaX = e.getX() - oldMouseX;
    int deltaY = e.getY() - oldMouseY;
    bool updated = false;
    double senseFactor = deltaMovement / 5.0;

    deltaX = java::Math::max(-5, java::Math::min(5, deltaX));
    deltaY = java::Math::max(-5, java::Math::min(5, deltaY));

    Matrix4x4d R;
    Matrix4x4d DR;
    Vector3Dd eyePosition = camera->getPosition();
    Vector3Dd focusedPosition = camera->getFocusedPosition();

    int modifiers = e.getModifiers();

    R = camera->getRotation();
    Vector3Dd u(R.get(0, 0), R.get(1, 0), R.get(2, 0));
    Vector3Dd v(R.get(0, 1), R.get(1, 1), R.get(2, 1));
    Vector3Dd w(R.get(0, 2), R.get(1, 2), R.get(2, 2));

    if ( (modifiers & MouseEvent::BUTTON1_DOWN_MASK) != 0 ) {
        double ax = -java::Math::min(2.0, 0.01 * deltaX);
        double ay = java::Math::min(2.0, 0.01 * deltaY);
        updated = orbitAroundPointOfInterest(ax, ay);
    }
    else if ( (modifiers & MouseEvent::BUTTON2_DOWN_MASK) != 0 ) {
        eyePosition = eyePosition.subtract(v.multiply(senseFactor * ((double)deltaX)));
        eyePosition = eyePosition.subtract(w.multiply(senseFactor * ((double)deltaY)));
        focusedPosition = focusedPosition.subtract(v.multiply(senseFactor * ((double)deltaX)));
        focusedPosition = focusedPosition.subtract(w.multiply(senseFactor * ((double)deltaY)));
        updated = true;
    }
    else if ( (modifiers & MouseEvent::BUTTON3_DOWN_MASK) != 0 ) {
        eyePosition = eyePosition.subtract(u.multiply(senseFactor * ((double)deltaY)));
        double ax = java::Math::min(2.0, 0.01 * deltaX);
        DR = Matrix4x4d().axisRotation(ax, u.x(), u.y(), u.z());
        R = DR.multiply(R);
        updated = true;
    }

    if ( (modifiers & MouseEvent::BUTTON1_DOWN_MASK) == 0 ) {
        camera->setPosition(eyePosition);
        camera->setFocusedPositionMaintainingOrthogonality(focusedPosition);
        camera->setRotation(R);
    }

    oldMouseX = e.getX();
    oldMouseY = e.getY();
    return updated;
}

bool CameraControllerOrbiter::processMouseWheelEvent(const MouseEvent& e)
{
    double fov = camera->getFov();
    double orthogonalZoom = camera->getOrthogonalZoom();
    const double epsilon = 0.0001;

    int clicks = e.getClicks();

    if ( clicks > 0 ) {
        if ( camera->getProjectionMode() == Camera::PROJECTION_MODE_ORTHOGONAL ) {
            orthogonalZoom /= 2 * clicks;
        }
        else {
            if ( fov < 0.1 - epsilon ) fov += 0.1 * clicks;
            else if ( fov < 1 - epsilon ) fov += clicks;
            else if ( fov < 175 - epsilon ) fov += 5 * clicks;
        }
    }
    else if ( clicks < 0 ) {
        if ( camera->getProjectionMode() == Camera::PROJECTION_MODE_ORTHOGONAL ) {
            orthogonalZoom *= 2 * (-clicks);
        }
        else {
            if ( fov > 5 + epsilon ) fov += 5 * clicks;
            else if ( fov > 1 + epsilon ) fov += clicks;
            else if ( fov > 0.1 + epsilon ) fov += 0.1 * clicks;
        }
    }

    camera->setFov(fov);
    camera->setOrthogonalZoom(orthogonalZoom);

    return true;
}

Camera* CameraControllerOrbiter::getCamera()
{
    return camera;
}

void CameraControllerOrbiter::setCamera(Camera* cam)
{
    camera = cam;
}

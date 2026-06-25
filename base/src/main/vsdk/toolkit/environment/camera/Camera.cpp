#include <cmath>
#include <cstdio>

#include "java/lang/String.h"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/camera/Camera.h"
#include "vsdk/toolkit/environment/camera/CameraSnapshot.h"
static const double PI = 3.14159265358979323846;

static inline double degreesToRadians(double degrees) {
    return degrees * PI / 180.0;
}

static java::String doubleToStr(double val) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%g", val);
    return java::String(buf);
}

Camera::Camera()
    : up(0, 0, 1),
      front(0, 1, 0),
      left(-1, 0, 0),
      eyePosition(0, -5, 1),
      focalDistance(10),
      projectionMode(PROJECTION_MODE_PERSPECTIVE),
      fov(60),
      orthogonalZoom(1),
      nearPlaneDistance(0.05),
      farPlaneDistance(100),
      viewportXSize(320),
      viewportYSize(320),
      modificationVersion(0) {
    updateVectors();
}

Camera::Camera(const Camera& other)
    : up(other.up),
      front(other.front),
      left(other.left),
      eyePosition(other.eyePosition),
      focalDistance(other.focalDistance),
      projectionMode(other.projectionMode),
      fov(other.fov),
      orthogonalZoom(other.orthogonalZoom),
      nearPlaneDistance(other.nearPlaneDistance),
      farPlaneDistance(other.farPlaneDistance),
      name(other.name),
      viewportXSize(other.viewportXSize),
      viewportYSize(other.viewportYSize),
      modificationVersion(0) {
    updateVectors();
}

Matrix4x4d Camera::getNormalizingTransformation() const {
    return normalizingTransformation;
}

long Camera::getModificationVersion() const {
    return modificationVersion;
}

void Camera::markModified() {
    modificationVersion++;
}

java::String Camera::getName() const {
    return name;
}

void Camera::setName(const java::String& n) {
    name = n;
    markModified();
}

double Camera::getViewportXSize() const {
    return viewportXSize;
}

double Camera::getViewportYSize() const {
    return viewportYSize;
}

Vector3Dd Camera::getPosition() const {
    return eyePosition;
}

void Camera::setPosition(const Vector3Dd& pos) {
    eyePosition = Vector3Dd(pos);
    markModified();
}

Vector3Dd Camera::getFocusedPosition() const {
    Vector3Dd partial = front.multiply(focalDistance);
    return eyePosition.add(partial);
}

void Camera::setFocusedPositionDirect(const Vector3Dd& focusedPosition) {
    Vector3Dd partial = focusedPosition.subtract(eyePosition);
    front = Vector3Dd(partial);
    focalDistance = front.length();
    front = front.normalized();
    markModified();
}

void Camera::setFocusedPositionMaintainingOrthogonality(const Vector3Dd& focusedPosition) {
    front = focusedPosition.subtract(eyePosition);
    focalDistance = front.length();
    front = front.normalized();

    left = up.crossProduct(front);
    left = left.normalized();

    up = front.crossProduct(left);
    up = up.normalized();
    markModified();
}

Vector3Dd Camera::getUp() const {
    return up;
}

Vector3Dd Camera::getUpWithScale() const {
    return upWithScale;
}

Vector3Dd Camera::getRightWithScale() const {
    return rightWithScale;
}

Vector3Dd Camera::getFront() const {
    return front;
}

Vector3Dd Camera::getLeft() const {
    return left;
}

void Camera::setUpDirect(const Vector3Dd& newUp) {
    up = Vector3Dd(newUp);
    markModified();
}

void Camera::setLeftDirect(const Vector3Dd& newLeft) {
    left = Vector3Dd(newLeft);
    markModified();
}

void Camera::setUpMaintainingOrthogonality(const Vector3Dd& newUp) {
    Vector3Dd normalizedUp = newUp.normalized();

    left = normalizedUp.crossProduct(front);
    left = left.normalized();

    up = front.crossProduct(left);
    up = up.normalized();
    markModified();
}

double Camera::getFov() const {
    return fov;
}

void Camera::setFov(double newFov) {
    fov = newFov;
    markModified();
}

double Camera::getNearPlaneDistance() const {
    return nearPlaneDistance;
}

void Camera::setNearPlaneDistance(double newNearPlaneDistance) {
    nearPlaneDistance = newNearPlaneDistance;
    markModified();
}

double Camera::getFarPlaneDistance() const {
    return farPlaneDistance;
}

void Camera::setFarPlaneDistance(double newFarPlaneDistance) {
    farPlaneDistance = newFarPlaneDistance;
    markModified();
}

int Camera::getProjectionMode() const {
    return projectionMode;
}

void Camera::setProjectionMode(int newProjectionMode) {
    projectionMode = newProjectionMode;
    markModified();
}

double Camera::getOrthogonalZoom() const {
    return orthogonalZoom;
}

void Camera::setOrthogonalZoom(double newOrthogonalZoom) {
    orthogonalZoom = newOrthogonalZoom;
    markModified();
}

void Camera::updateViewportResize(int dx, int dy) {
    viewportXSize = dx;
    viewportYSize = dy;
    updateVectors();
    markModified();
}

void Camera::updateVectors() {
    up = up.normalized();
    left = left.normalized();
    front = front.normalized();

    double fovFactor = viewportXSize / viewportYSize;
    _dir = front.multiply(0.5);
    upWithScale = up.multiply(std::tan(degreesToRadians(fov / 2.0)));
    rightWithScale = left.multiply(-fovFactor * std::tan(degreesToRadians(fov / 2.0)));

    Vector3Dd VRP = eyePosition.add(front.multiply(nearPlaneDistance));
    Matrix4x4d T1 = Matrix4x4d().translation(VRP.multiply(-1));

    Matrix4x4d R1 = getRotation();
    R1 = R1.invert();

    Matrix4x4d R2 = Matrix4x4d().eulerAnglesRotation(
        degreesToRadians(90),
        degreesToRadians(-90),
        0);
    Matrix4x4d RTOTAL = R2.multiply(R1);

    Matrix4x4d T2 = Matrix4x4d().translation(0, 0, -nearPlaneDistance);

    Matrix4x4d S1 = Matrix4x4d();
    double ddx = rightWithScale.length();
    double ddy = upWithScale.length();
    S1 = S1.scale(ddx, ddy, 1);
    S1 = S1.invert();

    Matrix4x4d S2 = Matrix4x4d();
    Matrix4x4d T3 = Matrix4x4d();

    normalizingTransformation = T3.multiply(S2.multiply(S1.multiply(T2.multiply(RTOTAL.multiply(T1)))));
}

void Camera::setRotation(const Matrix4x4d& R) {
    up = up.withX(R.get(0, 2));
    up = up.withY(R.get(1, 2));
    up = up.withZ(R.get(2, 2));
    up = up.normalized();

    front = front.withX(R.get(0, 0));
    front = front.withY(R.get(1, 0));
    front = front.withZ(R.get(2, 0));
    front = front.normalized();

    left = left.withX(R.get(0, 1));
    left = left.withY(R.get(1, 1));
    left = left.withZ(R.get(2, 1));
    left = left.normalized();
    markModified();
}

Matrix4x4d Camera::getRotation() const {
    Matrix4x4d R = Matrix4x4d()
        .withVal(0, 0, front.x()).withVal(0, 1, left.x()).withVal(0, 2, up.x())
        .withVal(1, 0, front.y()).withVal(1, 1, left.y()).withVal(1, 2, up.y())
        .withVal(2, 0, front.z()).withVal(2, 1, left.z()).withVal(2, 2, up.z());
    return R;
}

Matrix4x4d Camera::calculateViewVolumeMatrix() const {
    double aspect = viewportXSize / viewportYSize;
    Matrix4x4d P = Matrix4x4d();

    switch (projectionMode) {
        case PROJECTION_MODE_ORTHOGONAL:
            P = P.orthogonalProjection(-aspect / orthogonalZoom,
                                       aspect / orthogonalZoom,
                                       -1 / orthogonalZoom, 1 / orthogonalZoom,
                                       nearPlaneDistance, farPlaneDistance);
            break;
        case PROJECTION_MODE_PERSPECTIVE: {
            double upDistance = nearPlaneDistance * std::tan(degreesToRadians(fov / 2.0));
            double downDistance = -upDistance;
            double leftDistance = aspect * downDistance;
            double rightDistance = aspect * upDistance;
            P = P.frustumProjection(leftDistance, rightDistance,
                                    downDistance, upDistance,
                                    nearPlaneDistance, farPlaneDistance);
            break;
        }
    }
    return P;
}

Matrix4x4d Camera::calculateTransformationMatrix() const {
    Matrix4x4d R1 = getRotation();
    R1 = R1.invert();

    Matrix4x4d T1 = Matrix4x4d().translation(-eyePosition.x(), -eyePosition.y(), -eyePosition.z());
    Matrix4x4d R_adic2 = Matrix4x4d().axisRotation(degreesToRadians(90), 0, 0, 1);
    Matrix4x4d R_adic1 = Matrix4x4d().axisRotation(degreesToRadians(-90), 1, 0, 0);

    Matrix4x4d R = R_adic1.multiply(R_adic2.multiply(R1.multiply(T1)));
    return R;
}

Matrix4x4d Camera::calculateProjectionMatrix() const {
    Matrix4x4d P = calculateViewVolumeMatrix();
    Matrix4x4d R = calculateTransformationMatrix();
    return P.multiply(R);
}

Ray Camera::generateRay(int x, int y) const
{
    double u = ((double)x - viewportXSize/2.0) / viewportXSize;
    double v = ((viewportYSize - (double)y - 1) - viewportYSize/2.0) / viewportYSize;

    if ( projectionMode == PROJECTION_MODE_ORTHOGONAL ) {
        double fovFactor = viewportXSize / viewportYSize;
        double duScale = (-fovFactor) * (2 * u / orthogonalZoom);
        double dvScale = 2 * v / orthogonalZoom;
        Vector3Dd origin(
            eyePosition.x() + left.x()*duScale + up.x()*dvScale,
            eyePosition.y() + left.y()*duScale + up.y()*dvScale,
            eyePosition.z() + left.z()*duScale + up.z()*dvScale);
        return Ray(origin, front);
    }

    Vector3Dd direction(
        rightWithScale.x()*u + upWithScale.x()*v + _dir.x(),
        rightWithScale.y()*u + upWithScale.y()*v + _dir.y(),
        rightWithScale.z()*u + upWithScale.z()*v + _dir.z());

    return Ray(eyePosition, direction);
}

float* Camera::toColumnMajorFloatArray() const {
    return calculateProjectionMatrix().exportToFloatArrayColumnOrder();
}

java::String Camera::toString() const {
    java::String msg = "<Camera>:\n";
    msg += "  - Name: \"" + name + "\"\n";

    if (projectionMode == PROJECTION_MODE_PERSPECTIVE) {
        msg += "  - Camera in PERSPECTIVE projection mode\n";
    } else if (projectionMode == PROJECTION_MODE_ORTHOGONAL) {
        msg += "  - Camera in PARALLEL projection mode\n";
        msg += "  - Orthogonal zoom = ";
        msg += doubleToStr(orthogonalZoom);
        msg += "\n";
    } else {
        msg += "  - UNKNOWN Camera projection mode!\n";
    }

    msg += "  - eyePosition(x, y, z) = (";
    msg += doubleToStr(eyePosition.x());
    msg += ", ";
    msg += doubleToStr(eyePosition.y());
    msg += ", ";
    msg += doubleToStr(eyePosition.z());
    msg += ")\n";

    Vector3Dd focusedPos = eyePosition.add(front.multiply(focalDistance));
    msg += "  - focusedPointPosition(x, y, z) = (";
    msg += doubleToStr(focusedPos.x());
    msg += ", ";
    msg += doubleToStr(focusedPos.y());
    msg += ", ";
    msg += doubleToStr(focusedPos.z());
    msg += ")\n";

    Matrix4x4d R = getRotation();
    double yaw = R.obtainEulerYawAngle();
    double pitch = R.obtainEulerPitchAngle();
    double roll = R.obtainEulerRollAngle();

    msg += "  - Rotation yaw/pitch/roll (RAD): <";
    msg += doubleToStr(yaw);
    msg += ", ";
    msg += doubleToStr(pitch);
    msg += ", ";
    msg += doubleToStr(roll);
    msg += ">\n";

    msg += "  - Reference frame:\n";
    msg += "    . Vector UP = (";
    msg += doubleToStr(up.x());
    msg += ", ";
    msg += doubleToStr(up.y());
    msg += ", ";
    msg += doubleToStr(up.z());
    msg += ") (length ";
    msg += doubleToStr(up.length());
    msg += ")\n";
    msg += "    . Vector FRONT = (";
    msg += doubleToStr(front.x());
    msg += ", ";
    msg += doubleToStr(front.y());
    msg += ", ";
    msg += doubleToStr(front.z());
    msg += ") (length ";
    msg += doubleToStr(front.length());
    msg += ")\n";
    msg += "    . Vector LEFT = (";
    msg += doubleToStr(left.x());
    msg += ", ";
    msg += doubleToStr(left.y());
    msg += ", ";
    msg += doubleToStr(left.z());
    msg += ") (length ";
    msg += doubleToStr(left.length());
    msg += ")\n";

    msg += "  - fov = ";
    msg += doubleToStr(fov);
    msg += "\n";
    msg += "  - nearPlaneDistance = ";
    msg += doubleToStr(nearPlaneDistance);
    msg += "\n";
    msg += "  - farPlaneDistance = ";
    msg += doubleToStr(farPlaneDistance);
    msg += "\n";
    msg += "  - Viewport size in pixels = (";
    msg += doubleToStr(viewportXSize);
    msg += ", ";
    msg += doubleToStr(viewportYSize);
    msg += ")\n";

    return msg;
}

CameraSnapshot* Camera::exportToCameraSnapshot() const
{
    return exportToCameraSnapshot(static_cast<int>(viewportXSize), static_cast<int>(viewportYSize));
}

CameraSnapshot* Camera::exportToCameraSnapshot(int viewportXSizeIn, int viewportYSizeIn) const
{
    Camera tmp(*this);
    tmp.viewportXSize = viewportXSizeIn;
    tmp.viewportYSize = viewportYSizeIn;
    tmp.updateVectors();
    return new CameraSnapshot(
        tmp.eyePosition,
        tmp.front,
        tmp.left,
        tmp.up,
        tmp.projectionMode,
        tmp.orthogonalZoom,
        tmp.viewportXSize,
        tmp.viewportYSize,
        tmp._dir,
        tmp.upWithScale,
        tmp.rightWithScale);
}

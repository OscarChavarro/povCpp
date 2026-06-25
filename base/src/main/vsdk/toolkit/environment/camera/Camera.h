#ifndef CAMERA_H
#define CAMERA_H

#include "java/lang/String.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
class CameraSnapshot;

class Camera {
public:
    static const int OPCODE_FAR = (0x01 << 1);
    static const int OPCODE_NEAR = (0x01 << 2);
    static const int OPCODE_RIGHT = (0x01 << 3);
    static const int OPCODE_LEFT = (0x01 << 4);
    static const int OPCODE_UP = (0x01 << 5);
    static const int OPCODE_DOWN = (0x01 << 6);

    static const int PROJECTION_MODE_ORTHOGONAL = 4;
    static const int PROJECTION_MODE_PERSPECTIVE = 5;

    Camera();
    Camera(const Camera& other);

    Matrix4x4d getNormalizingTransformation() const;
    long getModificationVersion() const;

    java::String getName() const;
    void setName(const java::String& name);

    double getViewportXSize() const;
    double getViewportYSize() const;

    Vector3Dd getPosition() const;
    void setPosition(const Vector3Dd& pos);

    Vector3Dd getFocusedPosition() const;
    void setFocusedPositionDirect(const Vector3Dd& focusedPosition);
    void setFocusedPositionMaintainingOrthogonality(const Vector3Dd& focusedPosition);

    Vector3Dd getUp() const;
    Vector3Dd getUpWithScale() const;
    Vector3Dd getRightWithScale() const;
    Vector3Dd getFront() const;
    Vector3Dd getLeft() const;

    void setUpDirect(const Vector3Dd& up);
    void setLeftDirect(const Vector3Dd& left);
    void setUpMaintainingOrthogonality(const Vector3Dd& up);

    double getFov() const;
    void setFov(double fov);

    double getNearPlaneDistance() const;
    void setNearPlaneDistance(double nearPlaneDistance);

    double getFarPlaneDistance() const;
    void setFarPlaneDistance(double farPlaneDistance);

    int getProjectionMode() const;
    void setProjectionMode(int projectionMode);

    double getOrthogonalZoom() const;
    void setOrthogonalZoom(double orthogonalZoom);

    void updateViewportResize(int dx, int dy);
    void updateVectors();

    void setRotation(const Matrix4x4d& R);
    Matrix4x4d getRotation() const;

    Matrix4x4d calculateViewVolumeMatrix() const;
    Matrix4x4d calculateTransformationMatrix() const;
    Matrix4x4d calculateProjectionMatrix() const;
    Ray generateRay(int x, int y) const;

    float* toColumnMajorFloatArray() const;

    java::String toString() const;
    CameraSnapshot* exportToCameraSnapshot() const;
    CameraSnapshot* exportToCameraSnapshot(int viewportXSize, int viewportYSize) const;

private:
    Vector3Dd up;
    Vector3Dd front;
    Vector3Dd left;
    Vector3Dd eyePosition;
    double focalDistance;
    int projectionMode;

    double fov;
    double orthogonalZoom;
    double nearPlaneDistance;
    double farPlaneDistance;

    java::String name;

    double viewportXSize;
    double viewportYSize;

    Vector3Dd dx, dy, _dir, upWithScale, rightWithScale;
    Matrix4x4d normalizingTransformation;
    mutable long modificationVersion;

    void markModified();
};


#endif // CAMERA_H

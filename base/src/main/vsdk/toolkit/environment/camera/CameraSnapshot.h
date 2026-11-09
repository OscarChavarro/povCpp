#ifndef __CAMERA_SNAPSHOT__
#define __CAMERA_SNAPSHOT__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
class CameraSnapshot {
private:
    Vector3Dd eyePosition;
    Vector3Dd front;
    Vector3Dd left;
    Vector3Dd up;
    int projectionMode;
    double orthogonalZoom;
    double viewportXSize;
    double viewportYSize;
    Vector3Dd dir;
    Vector3Dd upWithScale;
    Vector3Dd rightWithScale;
    double nearPlaneDistance;
    double farPlaneDistance;

public:
    /**
    Snapshot with the default clipping planes of `Camera` (0.05 and 100).
    */
    CameraSnapshot(
        const Vector3Dd& eyePosition,
        const Vector3Dd& front,
        const Vector3Dd& left,
        const Vector3Dd& up,
        int projectionMode,
        double orthogonalZoom,
        double viewportXSize,
        double viewportYSize,
        const Vector3Dd& dir,
        const Vector3Dd& upWithScale,
        const Vector3Dd& rightWithScale)
        : eyePosition(eyePosition), front(front), left(left), up(up), projectionMode(projectionMode),
          orthogonalZoom(orthogonalZoom), viewportXSize(viewportXSize), viewportYSize(viewportYSize),
          dir(dir), upWithScale(upWithScale), rightWithScale(rightWithScale),
          nearPlaneDistance(0.05), farPlaneDistance(100.0)
    {
    }

    /**
    @param nearPlaneDistance distance from the eye to the near clipping plane
    @param farPlaneDistance distance from the eye to the far clipping plane
    (the other parameters as in the constructor without clipping planes)
    */
    CameraSnapshot(
        const Vector3Dd& eyePosition,
        const Vector3Dd& front,
        const Vector3Dd& left,
        const Vector3Dd& up,
        int projectionMode,
        double orthogonalZoom,
        double viewportXSize,
        double viewportYSize,
        const Vector3Dd& dir,
        const Vector3Dd& upWithScale,
        const Vector3Dd& rightWithScale,
        double nearPlaneDistance,
        double farPlaneDistance)
        : eyePosition(eyePosition), front(front), left(left), up(up), projectionMode(projectionMode),
          orthogonalZoom(orthogonalZoom), viewportXSize(viewportXSize), viewportYSize(viewportYSize),
          dir(dir), upWithScale(upWithScale), rightWithScale(rightWithScale),
          nearPlaneDistance(nearPlaneDistance), farPlaneDistance(farPlaneDistance)
    {
    }

    const Vector3Dd& getEyePosition() const { return eyePosition; }
    const Vector3Dd& getFront() const { return front; }
    const Vector3Dd& getLeft() const { return left; }
    const Vector3Dd& getUp() const { return up; }
    int getProjectionMode() const { return projectionMode; }
    double getOrthogonalZoom() const { return orthogonalZoom; }
    double getViewportXSize() const { return viewportXSize; }
    double getViewportYSize() const { return viewportYSize; }
    const Vector3Dd& getDir() const { return dir; }
    const Vector3Dd& getUpWithScale() const { return upWithScale; }
    const Vector3Dd& getRightWithScale() const { return rightWithScale; }
    /** @return near plane distance, as used by `Camera::calculateViewVolumeMatrix` */
    double getNearPlaneDistance() const { return nearPlaneDistance; }
    /** @return far plane distance, as used by `Camera::calculateViewVolumeMatrix` */
    double getFarPlaneDistance() const { return farPlaneDistance; }
};


#endif

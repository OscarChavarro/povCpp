#ifndef __VSDK_TOOLKIT_ENVIRONMENT_CAMERA_CAMERASNAPSHOT_H__
#define __VSDK_TOOLKIT_ENVIRONMENT_CAMERA_CAMERASNAPSHOT_H__

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

public:
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
          dir(dir), upWithScale(upWithScale), rightWithScale(rightWithScale)
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
};


#endif

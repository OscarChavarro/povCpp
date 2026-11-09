#include <cmath>

#include "vsdk/toolkit/common/VSDKFatalException.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/environment/camera/Camera.h"
#include "vsdk/toolkit/environment/camera/CameraSnapshot.h"
#include "vsdk/toolkit/render/raytracing/DepthBufferEncoder.h"

DepthBufferEncoder::DepthBufferEncoder(DepthBufferMode mode, const CameraSnapshot* camera)
{
    init(mode, camera, 0.0, 1.0);
}

DepthBufferEncoder::DepthBufferEncoder(DepthBufferMode mode, const CameraSnapshot* camera,
                                       double depthRangeNear, double depthRangeFar)
{
    init(mode, camera, depthRangeNear, depthRangeFar);
}

void DepthBufferEncoder::init(DepthBufferMode modeIn, const CameraSnapshot* camera,
                              double depthRangeNearIn, double depthRangeFarIn)
{
    if ( camera == 0 ) {
        Logger::reportMessage("DepthBufferEncoder", Logger::ERROR, "DepthBufferEncoder",
            "camera can not be null");
        throw VSDKFatalException("camera can not be null");
    }
    if ( modeIn == DepthBufferMode::OPENGL_DEPTH &&
         !(camera->getNearPlaneDistance() < camera->getFarPlaneDistance()) ) {
        Logger::reportMessage("DepthBufferEncoder", Logger::ERROR, "DepthBufferEncoder",
            "OPENGL_DEPTH needs a camera with near plane distance < far plane distance");
        throw VSDKFatalException(
            "OPENGL_DEPTH needs a camera with near plane distance < far plane distance");
    }
    mode = modeIn;
    projectionMode = camera->getProjectionMode();
    nearPlaneDistance = camera->getNearPlaneDistance();
    farPlaneDistance = camera->getFarPlaneDistance();
    depthRangeNear = depthRangeNearIn;
    depthRangeFar = depthRangeFarIn;
    eyePosition = camera->getEyePosition();
    front = camera->getFront();
}

DepthBufferMode DepthBufferEncoder::getMode() const
{
    return mode;
}

float DepthBufferEncoder::encode(const Vector3Dd& origin, const Vector3Dd& unitDirection,
                                 double distance) const
{
    if ( mode != DepthBufferMode::OPENGL_DEPTH ) {
        return (float)distance;
    }
    if ( std::isinf(distance) || std::isnan(distance) ) {
        return (float)depthRangeFar;
    }
    double eyeDepth =
        (origin.x() - eyePosition.x()) * front.x() +
        (origin.y() - eyePosition.y()) * front.y() +
        (origin.z() - eyePosition.z()) * front.z() +
        distance * unitDirection.dotProduct(front);
    return (float)eyeDepthToWindowDepth(eyeDepth);
}

double DepthBufferEncoder::eyeDepthToWindowDepth(double eyeDepth) const
{
    double n = nearPlaneDistance;
    double f = farPlaneDistance;
    double ndcZ;

    if ( projectionMode == Camera::PROJECTION_MODE_ORTHOGONAL ) {
        // Row 3 of glOrtho applied to eye space z = -eyeDepth, w = 1
        ndcZ = (2.0 * eyeDepth - (f + n)) / (f - n);
    }
    else {
        // Row 3 of glFrustum over w = eyeDepth
        if ( eyeDepth <= 0.0 ) {
            return depthRangeNear;
        }
        ndcZ = ((f + n) / (f - n)) - (2.0 * f * n) / ((f - n) * eyeDepth);
    }
    if ( ndcZ < -1.0 ) {
        ndcZ = -1.0;
    }
    if ( ndcZ > 1.0 ) {
        ndcZ = 1.0;
    }
    return depthRangeNear + (depthRangeFar - depthRangeNear) * (ndcZ + 1.0) / 2.0;
}

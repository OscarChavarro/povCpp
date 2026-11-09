#ifndef __DEPTH_BUFFER_ENCODER__
#define __DEPTH_BUFFER_ENCODER__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/render/raytracing/DepthBufferMode.h"

class CameraSnapshot;

/**
Converts the distances of the primary rays of a raytracer into the values of
a depth buffer (see `DepthBufferMode`). The OpenGL conversion follows the
same projection matrices used by `Camera::calculateViewVolumeMatrix`
(`Matrix4x4d::frustumProjection` and `Matrix4x4d::orthogonalProjection`, as
`glFrustum` and `glOrtho`), so a depth encoded here compares correctly, with
the usual `GL_LESS` / `GL_LEQUAL` depth functions, against the fragments of
geometry rasterized with that camera.

An instance is immutable and can be shared among rendering threads.

C++ counterpart of Java's `vsdk.toolkit.render.raytracing.DepthBufferEncoder`.
*/
class DepthBufferEncoder {
  private:
    DepthBufferMode mode;
    int projectionMode;
    double nearPlaneDistance;
    double farPlaneDistance;
    double depthRangeNear;
    double depthRangeFar;
    Vector3Dd eyePosition;
    Vector3Dd front;

    void init(DepthBufferMode mode, const CameraSnapshot* camera,
              double depthRangeNear, double depthRangeFar);

  public:
    /**
    Encoder with the default OpenGL depth range [0, 1].
    @param mode kind of depth buffer to produce
    @param camera camera that generates the primary rays
    */
    DepthBufferEncoder(DepthBufferMode mode, const CameraSnapshot* camera);

    /**
    @param mode kind of depth buffer to produce
    @param camera camera that generates the primary rays
    @param depthRangeNear near value of `glDepthRange`
    @param depthRangeFar far value of `glDepthRange`
    */
    DepthBufferEncoder(DepthBufferMode mode, const CameraSnapshot* camera,
                       double depthRangeNear, double depthRangeFar);

    /**
    @return kind of depth buffer produced
    */
    DepthBufferMode getMode() const;

    /**
    Encodes the depth of a primary ray.
    @param origin origin of the primary ray
    @param unitDirection unit length direction of the primary ray
    @param distance distance from the origin to the nearest hit, or positive
    infinity if nothing was hit
    @return depth value for the buffer
    */
    float encode(const Vector3Dd& origin, const Vector3Dd& unitDirection,
                 double distance) const;

    /**
    Converts a depth along the viewing direction of the camera (positive in
    front of the eye) into an OpenGL window depth, clamped to the depth range.
    @param eyeDepth distance from the eye to the point, measured along the
    front vector of the camera
    @return window space depth
    */
    double eyeDepthToWindowDepth(double eyeDepth) const;
};

#endif

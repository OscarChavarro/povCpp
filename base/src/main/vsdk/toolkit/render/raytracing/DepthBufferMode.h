#ifndef __DEPTH_BUFFER_MODE__
#define __DEPTH_BUFFER_MODE__

/**
Kinds of depth buffer a raytracer can export together with its color image
(see `ParallelRaytracer::setDepthBufferMode` and `DepthBufferEncoder`).

C++ counterpart of Java's `vsdk.toolkit.render.raytracing.DepthBufferMode`.
*/
enum class DepthBufferMode {
    /** No depth buffer is exported */
    NONE,

    /**
    Native distances (identity transformation): the world space distance
    from the origin of the primary ray to its nearest hit. Pixels where the
    primary ray hits nothing get positive infinity.
    */
    RAY_DISTANCE,

    /**
    Window space depth values, as the ones OpenGL stores in its depth buffer
    for the same camera: the eye space depth is transformed by the projection
    matrix of the camera (`Camera::calculateViewVolumeMatrix`, built from its
    near and far plane distances), divided by w, and mapped by
    `glDepthRange` (by default [0, 1]). Values are clamped to that range, and
    pixels where the primary ray hits nothing get the far value of the range,
    as a cleared OpenGL depth buffer. This buffer can be loaded in the
    OpenGL depth buffer to mix the raytraced image with rasterized geometry.
    */
    OPENGL_DEPTH
};

#endif

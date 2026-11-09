#ifndef __REFERENCE_FRAME_GIZMO__
#define __REFERENCE_FRAME_GIZMO__

#include "java/lang/String.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/gui/gizmo/Gizmo.h"

class ViewportElementScaler;

/**
Gizmo that represents the orientation of the world reference frame (X, Y and
Z axes) as seen from a camera. It is designed to be drawn as a small
indicator over a corner of a viewport, as done by 3D modeling programs.

This class holds the model needed to draw the gizmo (size, axes, colors,
labels and label positions) and the operations to estimate the rotation the
axes must have to be shown from a given camera. It does not depend on any
rendering technology: renderers are responsible for the actual painting.

The gizmo is defined in a canonical space where the origin is at the center
of the square area of `getSizeInPixels()` pixels devoted to it, and the
visible axes have length `getAxisLength()` (the area covers the range
[-1, 1] in each direction).
*/
class ReferenceFrameGizmo : public Gizmo {
public:
    static const int AXIS_X = 0;
    static const int AXIS_Y = 1;
    static const int AXIS_Z = 2;
    static const int NUMBER_OF_AXES = 3;

    static const int DEFAULT_SIZE_IN_PIXELS = 64;
    static const double DEFAULT_AXIS_LENGTH;
    static const double DEFAULT_LINE_WIDTH;

private:
    int sizeInPixels;
    double axisLength;
    double lineWidth;
    bool visible;
    ColorRgb axisColors[NUMBER_OF_AXES];
    java::String axisLabels[NUMBER_OF_AXES];

    static Vector3Dd unitVector(int axis);
    static void checkAxis(int axis);

public:
    ReferenceFrameGizmo();
    virtual ~ReferenceFrameGizmo() {}

    /**
    @return the width and height, in pixels, of the square area where the
    gizmo is drawn
    */
    int getSizeInPixels() const;

    /**
    @param sizeInPixels width and height, in pixels, of the square area where
    the gizmo is drawn; not positive values are ignored
    */
    void setSizeInPixels(int sizeInPixels);
    double getAxisLength() const;

    /**
    @param axisLength length of the axes, in the canonical space of the
    gizmo; not positive values are ignored
    */
    void setAxisLength(double axisLength);
    double getLineWidth() const;

    /**
    @param lineWidth width, in pixels, of the lines that draw the axes; not
    positive values are ignored
    */
    void setLineWidth(double lineWidth);

    /**
    Sets the size of the area and the line width of the gizmo to the values
    that make it look proportional to the screen resolution: the default
    values (designed for legacy resolutions) multiplied by the scale of the
    given scaler.
    @param scaler scaler informed of the resolution of the screen
    */
    void applyScale(const ViewportElementScaler* scaler);
    bool isVisible() const;
    void setVisible(bool visible);

    /**
    @param axis one of `AXIS_X`, `AXIS_Y` or `AXIS_Z`
    @return the end point of the axis, in the canonical space of the gizmo
    */
    Vector3Dd getAxisEnd(int axis) const;

    /**
    @param axis one of `AXIS_X`, `AXIS_Y` or `AXIS_Z`
    @return the color used to draw the axis and its label
    */
    const ColorRgb& getAxisColor(int axis) const;

    /**
    @param axis one of `AXIS_X`, `AXIS_Y` or `AXIS_Z`
    @return the text that names the axis
    */
    const java::String& getAxisLabel(int axis) const;

    /**
    @return the position, in the canonical space of the gizmo, where the
    label of the axis must be anchored: the end of the axis
    */
    Vector3Dd getLabelPosition(int axis) const;

    /**
    Estimates the rotation to apply to the axes of the gizmo (defined in
    world space) so they are seen from a camera with the given orientation:
    the inverse of the camera rotation, composed with the change of
    convention between the camera frame (front, left, up) and the canonical
    space of the gizmo, where the screen plane is XY and Z points to the
    viewer.
    @param cameraRotation rotation of the camera, as given by
    `Camera::getRotation()`
    @return the rotation matrix to apply to the axes of the gizmo
    */
    Matrix4x4d estimateOrientation(const Matrix4x4d& cameraRotation) const;

    /**
    Estimates the direction in which an axis of the gizmo points, once the
    orientation for the given camera is applied.
    @return an unit vector with the estimated direction of the axis
    */
    Vector3Dd estimateAxisDirection(int axis,
                                    const Matrix4x4d& cameraRotation) const;

    /**
    Builds the geometry to draw an axis as a line with thickness, seen from a
    camera: a triangle strip (a rectangle of 4 vertices, in strip order) in
    the canonical space of the gizmo, over the plane of the screen. The width
    of the rectangle is `getLineWidth()` pixels, and it is extended half of
    that width at both ends (square caps), so the joint of the axes at the
    origin has no gaps and an axis pointing to the viewer is seen as a dot.
    @param axis one of `AXIS_X`, `AXIS_Y` or `AXIS_Z`
    @param cameraRotation rotation of the camera
    @param outStrip the 4 vertices of the triangle strip; the z coordinates
    keep the depth of the ends of the axis
    */
    void buildAxisStrip(int axis, const Matrix4x4d& cameraRotation,
                        Vector3Dd outStrip[4]) const;
};

#endif

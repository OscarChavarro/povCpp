#include <cmath>

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/VSDKFatalException.h"
#include "vsdk/toolkit/gui/gizmo/ReferenceFrameGizmo.h"
#include "vsdk/toolkit/gui/viewport/ViewportElementScaler.h"

const double ReferenceFrameGizmo::DEFAULT_AXIS_LENGTH = 1.0;
const double ReferenceFrameGizmo::DEFAULT_LINE_WIDTH = 1.0;

ReferenceFrameGizmo::ReferenceFrameGizmo()
    : sizeInPixels(DEFAULT_SIZE_IN_PIXELS), axisLength(1.0), lineWidth(1.0),
      visible(true)
{
    axisColors[AXIS_X] = ColorRgb(0.78, 0, 0);
    axisColors[AXIS_Y] = ColorRgb(0, 0.61, 0);
    axisColors[AXIS_Z] = ColorRgb(0, 0, 0.76);
    axisLabels[AXIS_X] = "X";
    axisLabels[AXIS_Y] = "Y";
    axisLabels[AXIS_Z] = "Z";
}

int ReferenceFrameGizmo::getSizeInPixels() const
{
    return sizeInPixels;
}

void ReferenceFrameGizmo::setSizeInPixels(int sizeInPixels)
{
    if ( sizeInPixels > 0 ) {
        this->sizeInPixels = sizeInPixels;
    }
}

double ReferenceFrameGizmo::getAxisLength() const
{
    return axisLength;
}

void ReferenceFrameGizmo::setAxisLength(double axisLength)
{
    if ( axisLength > 0.0 ) {
        this->axisLength = axisLength;
    }
}

double ReferenceFrameGizmo::getLineWidth() const
{
    return lineWidth;
}

void ReferenceFrameGizmo::setLineWidth(double lineWidth)
{
    if ( lineWidth > 0.0 ) {
        this->lineWidth = lineWidth;
    }
}

void ReferenceFrameGizmo::applyScale(const ViewportElementScaler* scaler)
{
    if ( scaler == nullptr ) {
        return;
    }
    setSizeInPixels(scaler->scaleSize(DEFAULT_SIZE_IN_PIXELS));
    setLineWidth(scaler->scaleLength(DEFAULT_LINE_WIDTH));
}

bool ReferenceFrameGizmo::isVisible() const
{
    return visible;
}

void ReferenceFrameGizmo::setVisible(bool visible)
{
    this->visible = visible;
}

Vector3Dd ReferenceFrameGizmo::getAxisEnd(int axis) const
{
    checkAxis(axis);
    return unitVector(axis).multiply(axisLength);
}

const ColorRgb& ReferenceFrameGizmo::getAxisColor(int axis) const
{
    checkAxis(axis);
    return axisColors[axis];
}

const java::String& ReferenceFrameGizmo::getAxisLabel(int axis) const
{
    checkAxis(axis);
    return axisLabels[axis];
}

Vector3Dd ReferenceFrameGizmo::getLabelPosition(int axis) const
{
    return getAxisEnd(axis);
}

Matrix4x4d ReferenceFrameGizmo::estimateOrientation(
    const Matrix4x4d& cameraRotation) const
{
    Matrix4x4d viewRotation = Matrix4x4d()
        .axisRotation(M_PI / 2.0, -1, 0, 0)
        .multiply(Matrix4x4d().axisRotation(M_PI / 2.0, 0, 0, 1));

    return viewRotation.multiply(cameraRotation.invert());
}

Vector3Dd ReferenceFrameGizmo::estimateAxisDirection(
    int axis, const Matrix4x4d& cameraRotation) const
{
    checkAxis(axis);
    return estimateOrientation(cameraRotation).multiply(
        unitVector(axis)).normalized();
}

void ReferenceFrameGizmo::buildAxisStrip(int axis,
    const Matrix4x4d& cameraRotation, Vector3Dd outStrip[4]) const
{
    checkAxis(axis);

    Matrix4x4d orientation = estimateOrientation(cameraRotation);
    Vector3Dd start = orientation.multiply(Vector3Dd(0, 0, 0));
    Vector3Dd end = orientation.multiply(getAxisEnd(axis));

    double dx = end.x() - start.x();
    double dy = end.y() - start.y();
    double length = std::sqrt(dx * dx + dy * dy);

    if ( length < VSDK::EPSILON ) {
        dx = 1;
        dy = 0;
    }
    else {
        dx /= length;
        dy /= length;
    }

    // One canonical unit is half the size of the area, in pixels
    double halfWidth = lineWidth / sizeInPixels;
    double px = -dy * halfWidth;
    double py = dx * halfWidth;
    double sx = start.x() - dx * halfWidth;
    double sy = start.y() - dy * halfWidth;
    double ex = end.x() + dx * halfWidth;
    double ey = end.y() + dy * halfWidth;

    outStrip[0] = Vector3Dd(sx + px, sy + py, start.z());
    outStrip[1] = Vector3Dd(sx - px, sy - py, start.z());
    outStrip[2] = Vector3Dd(ex + px, ey + py, end.z());
    outStrip[3] = Vector3Dd(ex - px, ey - py, end.z());
}

Vector3Dd ReferenceFrameGizmo::unitVector(int axis)
{
    return Vector3Dd(
        axis == AXIS_X ? 1 : 0,
        axis == AXIS_Y ? 1 : 0,
        axis == AXIS_Z ? 1 : 0);
}

void ReferenceFrameGizmo::checkAxis(int axis)
{
    if ( axis < 0 || axis >= NUMBER_OF_AXES ) {
        throw VSDKFatalException("axis must be AXIS_X, AXIS_Y or AXIS_Z");
    }
}

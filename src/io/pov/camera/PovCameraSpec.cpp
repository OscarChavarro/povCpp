#include "vsdk/toolkit/environment/camera/Camera.h"

#include "io/pov/camera/PovCameraSpec.h"

PovCameraSpec::PovCameraSpec() :
    PovCameraSpec(Vector3Dd(0.0, 0.0, 0.0), Vector3Dd(0.0, 0.0, 1.0),
        Vector3Dd(0.0, 1.0, 0.0), Vector3Dd(1.33, 0.0, 0.0),
        Vector3Dd(0.0, 1.0, 0.0))
{
}

PovCameraSpec::PovCameraSpec(const Vector3Dd &location, const Vector3Dd &direction,
    const Vector3Dd &up, const Vector3Dd &right, const Vector3Dd &sky) :
    location(location),
    direction(direction),
    up(up),
    right(right),
    sky(sky)
{
}

void
PovCameraSpec::applyLinearTransformation(const Matrix4x4d &transformation)
{
    this->getLocation() = transformation.transpose().multiply(this->getLocation());
    this->getDirection() = transformation.transpose().multiply(this->getDirection());
    this->getUp() = transformation.transpose().multiply(this->getUp());
    this->getRight() = transformation.transpose().multiply(this->getRight());
}

void
PovCameraSpec::translate(Vector3Dd *vector)
{
    this->getLocation() = this->getLocation().add(*vector);
}

void
PovCameraSpec::rotate(Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Matrix4x4d transformationInverse;

    transformation.axisRotationRodrigues(&transformationInverse, vector);
    applyLinearTransformation(transformation);
}

void
PovCameraSpec::scale(Vector3Dd *vector)
{
    Matrix4x4d transformation = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    applyLinearTransformation(transformation);
}

CameraSnapshot
PovCameraSpec::bake() const
{
    const Vector3Dd front = direction.normalizedFast();
    const Vector3Dd left = right.normalizedFast().multiply(-1.0);
    const Vector3Dd upNormalized = up.normalizedFast();
    return CameraSnapshot(
        location,
        front,
        left,
        upNormalized,
        Camera::PROJECTION_MODE_PERSPECTIVE,
        1.0,
        0.0,
        0.0,
        direction,
        up,
        right);
}

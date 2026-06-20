#include "environment/geometry/element/Intersection.h"

#include "environment/geometry/SimpleBody.h"
#include "environment/material/PovrayMaterialUtils.h"

SimpleBody::SimpleBody(Geometry *geometry, Material *material, ColorRgba *shapeColor) :
    geometry(geometry),
    material(material),
    shapeColor(shapeColor)
{
}

ColorRgba *
SimpleBody::ensureShapeColor()
{
    if (shapeColor == nullptr) {
        shapeColor = new ColorRgba(0.0, 0.0, 0.0, 0.0);
    }
    return shapeColor;
}

void
SimpleBody::prependMaterialLayers(PovrayMaterial *newHead)
{
    PovrayMaterialUtils::prependTextureLayers(newHead, material);
}

int
SimpleBody::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    return getGeometry()->allIntersectionsForOwner(ray, depthQueue, this);
}

int
SimpleBody::inside(Vector3Dd *point)
{
    return getGeometry()->inside(point);
}

void
SimpleBody::normal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    getGeometry()->normal(result, intersectionPoint);
}

void
SimpleBody::normal(
    Vector3Dd *result,
    Vector3Dd *intersectionPoint,
    const RenderingConfiguration *config)
{
    getGeometry()->normal(result, intersectionPoint, config);
}

SimpleBody::SimpleBody(const SimpleBody &other) :
    geometry(other.geometry != nullptr ?
        (Geometry *)other.geometry->copy() : nullptr),
    material(other.material != nullptr ? other.material->copy() : nullptr),
    shapeColor(other.shapeColor),
    transform(other.transform),
    transformInverse(other.transformInverse)
{
}

void *
SimpleBody::copy()
{
    return new SimpleBody(*this);
}

void
SimpleBody::translate(Vector3Dd *vector)
{
    getGeometry()->translateGeometry(vector);
    if (getMaterial() != nullptr) {
        getMaterial()->translate(vector);
    }
    const Matrix4x4d delta = Matrix4x4d().translation(
        vector->x(), vector->y(), vector->z()).transpose();
    const Matrix4x4d deltaInverse = Matrix4x4d().translation(
        0.0 - vector->x(), 0.0 - vector->y(), 0.0 - vector->z()).transpose();
    getTransform() = getTransform().multiply(delta);
    getTransformInverse() = deltaInverse.multiply(getTransformInverse());
}

void
SimpleBody::rotate(Vector3Dd *vector)
{
    getGeometry()->rotateGeometry(vector);
    if (getMaterial() != nullptr) {
        getMaterial()->rotate(vector);
    }
    Matrix4x4d delta;
    Matrix4x4d deltaInverse;
    delta.axisRotationRodrigues(&deltaInverse, vector);
    getTransform() = getTransform().multiply(delta);
    getTransformInverse() = deltaInverse.multiply(getTransformInverse());
}

void
SimpleBody::scale(Vector3Dd *vector)
{
    getGeometry()->scaleGeometry(vector);
    if (getMaterial() != nullptr) {
        getMaterial()->scale(vector);
    }
    const Matrix4x4d delta = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    const Matrix4x4d deltaInverse = Matrix4x4d().scale(
        1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z());
    getTransform() = getTransform().multiply(delta);
    getTransformInverse() = deltaInverse.multiply(getTransformInverse());
}

void
SimpleBody::invert()
{
    getGeometry()->invertGeometry();
}

#include "java/util/PriorityQueue.txx"

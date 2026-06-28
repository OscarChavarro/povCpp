#include "io/pov/geometry/SimpleBodyBuilder.h"
#include "environment/geometry/GeometryTransformMutator.h"
#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"

SimpleBodyBuilder::SimpleBodyBuilder(
    Geometry *geometry, Material *material, ColorRgba *shapeColor) :
    geometry(geometry),
    material(material),
    shapeColor(shapeColor)
{
}

SimpleBodyBuilder::~SimpleBodyBuilder()
{
    delete geometry;
    delete material;
    delete shapeColor;
    delete transformation;
    delete transformationInverse;
}

void
SimpleBodyBuilder::ensureMatrices()
{
    if (transformation == nullptr) {
        transformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
        transformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
}

ColorRgba *
SimpleBodyBuilder::ensureShapeColor()
{
    if (shapeColor == nullptr) {
        shapeColor = new ColorRgba(0.0, 0.0, 0.0, 0.0);
    }
    return shapeColor;
}

Geometry *
SimpleBodyBuilder::releaseGeometry()
{
    Geometry *released = geometry;
    geometry = nullptr;
    return released;
}

Material *
SimpleBodyBuilder::releaseMaterial()
{
    Material *released = material;
    material = nullptr;
    return released;
}

ColorRgba *
SimpleBodyBuilder::releaseShapeColor()
{
    ColorRgba *released = shapeColor;
    shapeColor = nullptr;
    return released;
}

Matrix4x4d *
SimpleBodyBuilder::releaseTransformation()
{
    Matrix4x4d *released = transformation;
    transformation = nullptr;
    return released;
}

Matrix4x4d *
SimpleBodyBuilder::releaseTransformationInverse()
{
    Matrix4x4d *released = transformationInverse;
    transformationInverse = nullptr;
    return released;
}

void
SimpleBodyBuilder::prependMaterialLayers(Material *newHead)
{
    if (newHead != nullptr) {
        material = newHead->prependMaterialLayers(material);
    }
}

SimpleBodyBuilder::SimpleBodyBuilder(const SimpleBodyBuilder &other) :
    geometry(other.geometry != nullptr ?
        (Geometry *)other.geometry->copy() : nullptr),
    material(other.material != nullptr ? other.material->copy() : nullptr),
    shapeColor(other.shapeColor != nullptr ? new ColorRgba(*other.shapeColor) : nullptr),
    transformation(other.transformation != nullptr ?
        new Matrix4x4d(*other.transformation) : nullptr),
    transformationInverse(other.transformationInverse != nullptr ?
        new Matrix4x4d(*other.transformationInverse) : nullptr)
{
}

void
SimpleBodyBuilder::translate(Vector3Dd *vector)
{
    if (!GeometryTransformMutator::translateIfSupported(getGeometry(), vector)) {
        ensureMatrices();
        Matrix4x4d delta = Matrix4x4d().translation(
            vector->x(), vector->y(), vector->z()).transpose();
        Matrix4x4d deltaInverse = Matrix4x4d().translation(
            0.0 - vector->x(), 0.0 - vector->y(), 0.0 - vector->z()).transpose();
        *transformation = transformation->multiply(delta);
        *transformationInverse = deltaInverse.multiply(*transformationInverse);
    }
    if (getMaterial() != nullptr) {
        material = getMaterial()->translate(vector);
    }
}

void
SimpleBodyBuilder::translateOwnerOnly(Vector3Dd *vector)
{
    ensureMatrices();
    Matrix4x4d delta = Matrix4x4d().translation(
        vector->x(), vector->y(), vector->z()).transpose();
    Matrix4x4d deltaInverse = Matrix4x4d().translation(
        0.0 - vector->x(), 0.0 - vector->y(), 0.0 - vector->z()).transpose();
    *transformation = transformation->multiply(delta);
    *transformationInverse = deltaInverse.multiply(*transformationInverse);
}

void
SimpleBodyBuilder::rotate(Vector3Dd *vector)
{
    if (!GeometryTransformMutator::rotateIfSupported(getGeometry(), vector)) {
        ensureMatrices();
        Matrix4x4d delta;
        Matrix4x4d deltaInverse;
        delta.axisRotationRodrigues(&deltaInverse, vector);
        *transformation = transformation->multiply(delta);
        *transformationInverse = deltaInverse.multiply(*transformationInverse);
    }
    if (getMaterial() != nullptr) {
        material = getMaterial()->rotate(vector);
    }
}

void
SimpleBodyBuilder::rotateOwnerOnly(Vector3Dd *vector)
{
    ensureMatrices();
    Matrix4x4d delta;
    Matrix4x4d deltaInverse;
    delta.axisRotationRodrigues(&deltaInverse, vector);
    *transformation = transformation->multiply(delta);
    *transformationInverse = deltaInverse.multiply(*transformationInverse);
}

void
SimpleBodyBuilder::scale(Vector3Dd *vector)
{
    if (!GeometryTransformMutator::scaleIfSupported(getGeometry(), vector)) {
        ensureMatrices();
        Matrix4x4d delta = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
        Matrix4x4d deltaInverse = Matrix4x4d().scale(
            1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z());
        *transformation = transformation->multiply(delta);
        *transformationInverse = deltaInverse.multiply(*transformationInverse);
    }
    if (getMaterial() != nullptr) {
        material = getMaterial()->scale(vector);
    }
}

void
SimpleBodyBuilder::scaleOwnerOnly(Vector3Dd *vector)
{
    ensureMatrices();
    Matrix4x4d delta = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    Matrix4x4d deltaInverse = Matrix4x4d().scale(
        1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z());
    *transformation = transformation->multiply(delta);
    *transformationInverse = deltaInverse.multiply(*transformationInverse);
}

void
SimpleBodyBuilder::invert()
{
    getGeometry()->invertGeometry();
}

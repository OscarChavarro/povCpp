#include "io/pov/geometry/SimpleBodyBuilder.h"

SimpleBodyBuilder::SimpleBodyBuilder(
    TransformedGeometry *geometry, Material *material, ColorRgba *shapeColor) :
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
}

ColorRgba *
SimpleBodyBuilder::ensureShapeColor()
{
    if (shapeColor == nullptr) {
        shapeColor = new ColorRgba(0.0, 0.0, 0.0, 0.0);
    }
    return shapeColor;
}

TransformedGeometry *
SimpleBodyBuilder::releaseGeometry()
{
    TransformedGeometry *released = geometry;
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

void
SimpleBodyBuilder::prependMaterialLayers(Material *newHead)
{
    if (newHead != nullptr) {
        material = newHead->prependMaterialLayers(material);
    }
}

SimpleBodyBuilder::SimpleBodyBuilder(const SimpleBodyBuilder &other) :
    geometry(other.geometry != nullptr ?
        (TransformedGeometry *)other.geometry->copy() : nullptr),
    material(other.material != nullptr ? other.material->copy() : nullptr),
    shapeColor(other.shapeColor != nullptr ? new ColorRgba(*other.shapeColor) : nullptr)
{
}

void
SimpleBodyBuilder::translate(Vector3Dd *vector)
{
    getGeometry()->translateGeometry(vector);
    if (getMaterial() != nullptr) {
        material = getMaterial()->translate(vector);
    }
}

void
SimpleBodyBuilder::rotate(Vector3Dd *vector)
{
    getGeometry()->rotateGeometry(vector);
    if (getMaterial() != nullptr) {
        material = getMaterial()->rotate(vector);
    }
}

void
SimpleBodyBuilder::scale(Vector3Dd *vector)
{
    getGeometry()->scaleGeometry(vector);
    if (getMaterial() != nullptr) {
        material = getMaterial()->scale(vector);
    }
}

void
SimpleBodyBuilder::invert()
{
    getGeometry()->invertGeometry();
}

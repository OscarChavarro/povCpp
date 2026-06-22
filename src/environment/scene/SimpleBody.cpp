#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/scene/SimpleBody.h"

SimpleBody::SimpleBody(Geometry *geometry, Material *material, ColorRgba *shapeColor) :
    geometry(geometry),
    material(material),
    shapeColor(shapeColor)
{
}

SimpleBody::~SimpleBody()
{
    delete geometry;
    delete material;
    delete shapeColor;
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
SimpleBody::prependMaterialLayers(Material *newHead)
{
    if (newHead != nullptr) {
        material = newHead->prependMaterialLayers(material);
    }
}

int
SimpleBody::allIntersections(RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue)
{
    return getGeometry()->allIntersectionsForMaterial(ray, depthQueue, getMaterial());
}

int
SimpleBody::doContainmentTest(Vector3Dd *point)
{
    return getGeometry()->doContainmentTest(point);
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
    shapeColor(other.shapeColor != nullptr ? new ColorRgba(*other.shapeColor) : nullptr)
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
        material = getMaterial()->translate(vector);
    }
}

void
SimpleBody::rotate(Vector3Dd *vector)
{
    getGeometry()->rotateGeometry(vector);
    if (getMaterial() != nullptr) {
        material = getMaterial()->rotate(vector);
    }
}

void
SimpleBody::scale(Vector3Dd *vector)
{
    getGeometry()->scaleGeometry(vector);
    if (getMaterial() != nullptr) {
        material = getMaterial()->scale(vector);
    }
}

void
SimpleBody::invert()
{
    getGeometry()->invertGeometry();
}

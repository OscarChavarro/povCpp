#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/scene/SimpleBody.h"

SimpleBody::SimpleBody(TransformedGeometry *geometry, Material *material, ColorRgba *shapeColor) :
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

TransformedGeometry *
SimpleBody::releaseGeometry()
{
    TransformedGeometry *released = geometry;
    geometry = nullptr;
    return released;
}

Material *
SimpleBody::releaseMaterial()
{
    Material *released = material;
    material = nullptr;
    return released;
}

ColorRgba *
SimpleBody::releaseShapeColor()
{
    ColorRgba *released = shapeColor;
    shapeColor = nullptr;
    return released;
}

void
SimpleBody::prependMaterialLayers(Material *newHead)
{
    if (newHead != nullptr) {
        material = newHead->prependMaterialLayers(material);
    }
}

bool
SimpleBody::doIntersectionFirstHit(RayWithSegments *ray, IntersectionCandidate &out)
{
    return getGeometry()->doIntersectionFirstHit(ray, out);
}

int
SimpleBody::doIntersectionForAllRayCrossings(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    (void)materialOverride;
    return getGeometry()->doIntersectionForAllRayCrossings(ray, depthQueue, getMaterial());
}

int
SimpleBody::doContainmentTest(const Vector3Dd &point, double distanceTolerance)
{
    return getGeometry()->doContainmentTest(point, distanceTolerance);
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
    const PovRayRendererConfiguration *config)
{
    getGeometry()->normal(result, intersectionPoint, config);
}

SimpleBody::SimpleBody(const SimpleBody &other) :
    geometry(other.geometry != nullptr ?
        (TransformedGeometry *)other.geometry->copy() : nullptr),
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

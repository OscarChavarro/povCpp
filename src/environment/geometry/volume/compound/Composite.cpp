#include "common/Config.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/material/Material.h"
#include "java/util/PriorityQueue.txx"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.txx"
#include "environment/geometry/element/PriorityQueuePool.txx"

int
Composite::allIntersections(RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue)
{
    bool intersectionFound;
    bool anyIntersectionFound;
    TransformedGeometry *boundingShape;
    TransformedGeometry *clippingShape;
    IntersectionCandidate localIntersection;
    BoundedGeometry *localObject;
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue;
    Statistics &stats = *ray->getStatistics();

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShape = this->getBoundingShapes()[i];
        Vector3Dd rayOrigin(ray->getOrigin());

        stats.incrementBoundingRegionTests();
        {
            IntersectionCandidate _boundingHit;
            if (!boundingShape->doIntersectionFirstHit(ray, _boundingHit) &&
                boundingShape->doContainmentTest(rayOrigin, Config::SMALL_TOLERANCE) ==
                    Geometry::OUTSIDE) {
                return (false);
            }
        }
        stats.incrementBoundingRegionTestsSucceeded();
    }

    localDepthQueue = ray->getIntersectionQueuePool()->pop(128);
    anyIntersectionFound = false;

    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        localObject = this->getSimpleBodies()[i];

        localObject->allIntersections(ray, localDepthQueue);
    }

    for (const IntersectionCandidate& candidate : *localDepthQueue) {
        localIntersection = candidate;

        intersectionFound = true;

        for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
            clippingShape = this->getClippingShapes()[i];
            stats.incrementClippingRegionTests();
            if (clippingShape->doContainmentTest(localIntersection.getIntersection().point,
                    Config::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
                intersectionFound = false;
                break;
            }
            stats.incrementClippingRegionTestsSucceeded();
        }

        if (intersectionFound) {
            depthQueue->offer(localIntersection);
            anyIntersectionFound = true;
        }
    }
    localDepthQueue->clear();
    ray->getIntersectionQueuePool()->push(localDepthQueue);
    return (anyIntersectionFound);
}

int
BoundedGeometry::allIntersections(RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue)
{
    bool intersectionFound;
    bool anyIntersectionFound;
    IntersectionCandidate localIntersection;
    TransformedGeometry *boundingShape;
    TransformedGeometry *clippingShape;
    java::PriorityQueue<IntersectionCandidate> *localDepthQueue;
    Statistics &stats = *ray->getStatistics();

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShape = this->getBoundingShapes()[i];
        Vector3Dd rayOrigin(ray->getOrigin());

        stats.incrementBoundingRegionTests();
        {
            IntersectionCandidate _boundingHit;
            if (!boundingShape->doIntersectionFirstHit(ray, _boundingHit) &&
                boundingShape->doContainmentTest(rayOrigin, Config::SMALL_TOLERANCE) ==
                    Geometry::OUTSIDE) {
                return (false);
            }
        }
        stats.incrementBoundingRegionTestsSucceeded();
    }

    localDepthQueue = ray->getIntersectionQueuePool()->pop(128);
    anyIntersectionFound = false;
    this->getGeometry()->allIntersectionsForMaterial(
        ray, localDepthQueue, this->getGeometryMaterial());

    for (const IntersectionCandidate& candidate : *localDepthQueue) {
        localIntersection = candidate;

        localIntersection.getAttributes().setObjectTexture(this->getObjectTexture());
        localIntersection.getAttributes().setObjectColor(this->getObjectColor());
        localIntersection.getAttributes().setNoShadowFlag(this->getNoShadowFlag());
        intersectionFound = true;

        for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
            clippingShape = this->getClippingShapes()[i];

            stats.incrementClippingRegionTests();
            if (clippingShape->doContainmentTest(localIntersection.getIntersection().point,
                    Config::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
                intersectionFound = false;
                break;
            }
            stats.incrementClippingRegionTestsSucceeded();
        }

        if (intersectionFound) {
            depthQueue->offer(localIntersection);
            anyIntersectionFound = true;
        }
    }
    localDepthQueue->clear();
    ray->getIntersectionQueuePool()->push(localDepthQueue);
    return (anyIntersectionFound);
}

int
BoundedGeometry::allIntersectionsForMaterial(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *material)
{
    (void)material;
    return allIntersections(ray, depthQueue);
}

int
BoundedGeometry::doContainmentTest(const Vector3Dd &point, double distanceTolerance)
{
    TransformedGeometry *boundingShape;
    TransformedGeometry *clippingShape;

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShape = this->getBoundingShapes()[i];

        if (boundingShape->doContainmentTest(point, distanceTolerance) == OUTSIDE) {
            return OUTSIDE;
        }
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        clippingShape = this->getClippingShapes()[i];

        if (clippingShape->doContainmentTest(point, distanceTolerance) == OUTSIDE) {
            return OUTSIDE;
        }
    }

    if (this->getGeometry()->doContainmentTest(point, distanceTolerance) != OUTSIDE) {
        return INSIDE;
    }
    return OUTSIDE;
}

int
Composite::doContainmentTest(const Vector3Dd &point, double distanceTolerance)
{
    TransformedGeometry *boundingShape;
    TransformedGeometry *clippingShape;
    BoundedGeometry *localObject;

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShape = this->getBoundingShapes()[i];

        if (boundingShape->doContainmentTest(point, distanceTolerance) == OUTSIDE) {
            return OUTSIDE;
        }
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        clippingShape = this->getClippingShapes()[i];

        if (clippingShape->doContainmentTest(point, distanceTolerance) == OUTSIDE) {
            return OUTSIDE;
        }
    }

    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        localObject = this->getSimpleBodies()[i];

        if (localObject->doContainmentTest(point, distanceTolerance) != OUTSIDE) {
            return INSIDE;
        }
    }

    return OUTSIDE;
}

BoundedGeometry::BoundedGeometry(const BoundedGeometry &other) :
    geometry(other.getGeometry() != nullptr ?
        (TransformedGeometry *)other.getGeometry()->copy() : nullptr),
    geometryMaterial(other.getGeometryMaterial() != nullptr ?
        other.getGeometryMaterial()->copy() : nullptr),
    noShadowFlag(other.getNoShadowFlag()),
    objectColor(other.getObjectColor() != nullptr ?
        new ColorRgba(*other.getObjectColor()) : nullptr),
    objectTexture(other.getObjectTexture() != nullptr ?
        other.getObjectTexture()->copy() : nullptr)
{
    for (long int i = other.getBoundingShapes().size() - 1; i >= 0; i--) {
        boundingShapes.add(
            (TransformedGeometry *)other.getBoundingShapes()[i]->copy());
    }
    for (long int i = other.getClippingShapes().size() - 1; i >= 0; i--) {
        clippingShapes.add(
            (TransformedGeometry *)other.getClippingShapes()[i]->copy());
    }
}

BoundedGeometry::~BoundedGeometry()
{
    delete geometry;
    delete geometryMaterial;
    delete objectColor;
    // objectTexture may be a private clone (delete it) or an alias to a shared
    // constant such as the scene's default texture (do not delete, just close
    // out its alias bookkeeping). releaseFromOwner() encapsulates that decision
    // so this destructor needs to know nothing about the concrete material type.
    if (objectTexture != nullptr) {
        objectTexture->releaseFromOwner();
    }
    for (long int i = 0; i < boundingShapes.size(); i++) {
        delete boundingShapes[i];
    }
    for (long int i = 0; i < clippingShapes.size(); i++) {
        delete clippingShapes[i];
    }
}

void
BoundedGeometry::detachOwnership()
{
    geometry = nullptr;
    geometryMaterial = nullptr;
    objectColor = nullptr;
    objectTexture = nullptr;
    boundingShapes.clear();
    clippingShapes.clear();
}

void *
BoundedGeometry::copy()
{
    return new BoundedGeometry(*this);
}

Composite::Composite(const Composite &other) :
    BoundedGeometry(other)
{
    for (long int i = other.getSimpleBodies().size() - 1; i >= 0; i--) {
        simpleBodies.add(
            (BoundedGeometry *)other.getSimpleBodies()[i]->copy());
    }
}

Composite::~Composite()
{
    for (long int i = 0; i < simpleBodies.size(); i++) {
        delete simpleBodies[i];
    }
}

void
Composite::detachOwnership()
{
    BoundedGeometry::detachOwnership();
    simpleBodies.clear();
}

void *
Composite::copy()
{
    return new Composite(*this);
}

void
BoundedGeometry::translate(Vector3Dd *vector)
{
    TransformedGeometry *localShape;

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];

        localShape->translateGeometry(vector);
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];

        localShape->translateGeometry(vector);
    }

    if (this->getGeometry() != nullptr) {
        this->getGeometry()->translateGeometry(vector);
    }

    if (this->getGeometryMaterial() != nullptr) {
        geometryMaterial = this->getGeometryMaterial()->translate(vector);
    }

    if (this->getObjectTexture() != nullptr) {
        objectTexture = this->getObjectTexture()->translate(vector);
    }
}

void
BoundedGeometry::rotate(Vector3Dd *vector)
{
    TransformedGeometry *localShape;

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];

        localShape->rotateGeometry(vector);
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];

        localShape->rotateGeometry(vector);
    }

    if (this->getGeometry() != nullptr) {
        this->getGeometry()->rotateGeometry(vector);
    }

    if (this->getGeometryMaterial() != nullptr) {
        geometryMaterial = this->getGeometryMaterial()->rotate(vector);
    }

    if (this->getObjectTexture() != nullptr) {
        objectTexture = this->getObjectTexture()->rotate(vector);
    }
}

void
BoundedGeometry::scale(Vector3Dd *vector)
{
    TransformedGeometry *localShape;

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];

        localShape->scaleGeometry(vector);
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];

        localShape->scaleGeometry(vector);
    }

    if (this->getGeometry() != nullptr) {
        this->getGeometry()->scaleGeometry(vector);
    }

    if (this->getGeometryMaterial() != nullptr) {
        geometryMaterial = this->getGeometryMaterial()->scale(vector);
    }

    if (this->getObjectTexture() != nullptr) {
        objectTexture = this->getObjectTexture()->scale(vector);
    }
}

void
Composite::translate(Vector3Dd *vector)
{
    BoundedGeometry *localObject;
    TransformedGeometry *localShape;

    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        localObject = this->getSimpleBodies()[i];

        localObject->translate(vector);
    }

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];

        localShape->translateGeometry(vector);
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];

        localShape->translateGeometry(vector);
    }
}

void
Composite::rotate(Vector3Dd *vector)
{
    BoundedGeometry *localObject;
    TransformedGeometry *localShape;

    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        localObject = this->getSimpleBodies()[i];

        localObject->rotate(vector);
    }

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];

        localShape->rotateGeometry(vector);
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];

        localShape->rotateGeometry(vector);
    }
}

void
Composite::scale(Vector3Dd *vector)
{
    BoundedGeometry *localObject;
    TransformedGeometry *localShape;

    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        localObject = this->getSimpleBodies()[i];

        localObject->scale(vector);
    }

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];

        localShape->scaleGeometry(vector);
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];

        localShape->scaleGeometry(vector);
    }
}

void
BoundedGeometry::invert()
{
    TransformedGeometry *localShape;

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];
        localShape->invertGeometry();
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];
        localShape->invertGeometry();
    }
    if (this->getGeometry() != nullptr) {
        this->getGeometry()->invertGeometry();
    }
}

void
Composite::invert()
{
    BoundedGeometry *localObject;
    TransformedGeometry *localShape;

    for (long int i = this->getSimpleBodies().size() - 1; i >= 0; i--) {
        localObject = this->getSimpleBodies()[i];
        localObject->invert();
    }

    for (long int i = this->getBoundingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getBoundingShapes()[i];
        localShape->invertGeometry();
    }

    for (long int i = this->getClippingShapes().size() - 1; i >= 0; i--) {
        localShape = this->getClippingShapes()[i];
        localShape->invertGeometry();
    }
}

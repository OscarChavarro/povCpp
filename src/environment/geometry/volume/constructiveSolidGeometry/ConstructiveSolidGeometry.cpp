#include "java/util/ArrayList.txx"
#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"

ConstructiveSolidGeometry::ConstructiveSolidGeometry(BooleanSetOperations initialGeometryType) :
    geometryType(initialGeometryType)
{
}

ConstructiveSolidGeometry::~ConstructiveSolidGeometry()
{
    for (long int i = 0; i < shapes.size(); i++) {
        delete shapes[i];
    }
}

int
ConstructiveSolidGeometry::allIntersectionsForMaterial(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *material)
{
    (void)material;
    return allIntersections(ray, depthQueue);
}

void
ConstructiveSolidGeometry::translateGeometry(Vector3Dd *vector)
{
    TransformableElement *localShape;

    for (long int i = shapes.size() - 1; i >= 0; i--) {
        localShape = shapes[i];
        localShape->translate(vector);
    }
}

void
ConstructiveSolidGeometry::translate(Vector3Dd *vector)
{
    translateGeometry(vector);
}

void
ConstructiveSolidGeometry::rotateGeometry(Vector3Dd *vector)
{
    TransformableElement *localShape;

    for (long int i = shapes.size() - 1; i >= 0; i--) {
        localShape = shapes[i];
        localShape->rotate(vector);
    }
}

void
ConstructiveSolidGeometry::rotate(Vector3Dd *vector)
{
    rotateGeometry(vector);
}

void
ConstructiveSolidGeometry::scaleGeometry(Vector3Dd *vector)
{
    TransformableElement *localShape;

    for (long int i = shapes.size() - 1; i >= 0; i--) {
        localShape = shapes[i];
        localShape->scale(vector);
    }
}

void
ConstructiveSolidGeometry::scale(Vector3Dd *vector)
{
    scaleGeometry(vector);
}

void
ConstructiveSolidGeometry::invert()
{
    invertGeometry();
}

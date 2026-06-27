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
        delete shapeMaterials[i];
    }
}

void
ConstructiveSolidGeometry::translateGeometry(Vector3Dd *vector)
{
    for (long int i = shapes.size() - 1; i >= 0; i--) {
        shapes[i]->translateGeometry(vector);
        if (shapeMaterials[i] != nullptr) {
            shapeMaterials[i] = shapeMaterials[i]->translate(vector);
        }
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
    for (long int i = shapes.size() - 1; i >= 0; i--) {
        shapes[i]->rotateGeometry(vector);
        if (shapeMaterials[i] != nullptr) {
            shapeMaterials[i] = shapeMaterials[i]->rotate(vector);
        }
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
    for (long int i = shapes.size() - 1; i >= 0; i--) {
        shapes[i]->scaleGeometry(vector);
        if (shapeMaterials[i] != nullptr) {
            shapeMaterials[i] = shapeMaterials[i]->scale(vector);
        }
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

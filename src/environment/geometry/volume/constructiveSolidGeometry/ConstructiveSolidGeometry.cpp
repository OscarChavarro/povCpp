#include "java/util/ArrayList.txx"
#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"

ConstructiveSolidGeometry::ConstructiveSolidGeometry(BooleanSetOperations initialGeometryType) :
    geometryType(initialGeometryType)
{
}

ConstructiveSolidGeometry::~ConstructiveSolidGeometry()
{
    for (long int i = 0; i < operands.size(); i++) {
        delete operands[i];
    }
}

void
ConstructiveSolidGeometry::translateGeometry(Vector3Dd *vector)
{
    for (long int i = operands.size() - 1; i >= 0; i--) {
        operands[i]->translate(vector);
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
    for (long int i = operands.size() - 1; i >= 0; i--) {
        operands[i]->rotate(vector);
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
    for (long int i = operands.size() - 1; i >= 0; i--) {
        operands[i]->scale(vector);
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

AxisAlignedBox
ConstructiveSolidGeometry::getMinMax() const
{
    AxisAlignedBox result = AxisAlignedBox::empty();
    for (long int i = 0; i < operands.size(); i++) {
        result = result.enclosing(operands[i]->getGeometry()->getMinMax());
    }
    return result;
}

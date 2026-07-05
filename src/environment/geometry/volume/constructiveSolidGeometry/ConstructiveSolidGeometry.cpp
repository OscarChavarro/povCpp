#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "java/util/ArrayList.txx"

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
ConstructiveSolidGeometry::translate(Vector3Dd *vector)
{
    for (long int i = operands.size() - 1; i >= 0; i--) {
        operands[i]->translate(vector);
    }
}

void
ConstructiveSolidGeometry::rotate(Vector3Dd *vector)
{
    for (long int i = operands.size() - 1; i >= 0; i--) {
        operands[i]->rotate(vector);
    }
}

void
ConstructiveSolidGeometry::scale(Vector3Dd *vector)
{
    for (long int i = operands.size() - 1; i >= 0; i--) {
        operands[i]->scale(vector);
    }
}

void
ConstructiveSolidGeometry::invert()
{
    invertGeometry();
}

AxisAlignedBoundingBox
ConstructiveSolidGeometry::getMinMax() const
{
    AxisAlignedBoundingBox result = AxisAlignedBoundingBox::empty();
    for (long int i = 0; i < operands.size(); i++) {
        result = result.enclosing(operands[i]->getMinMax());
    }
    return result;
}

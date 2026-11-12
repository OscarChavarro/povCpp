#include "vsdk/toolkit/environment/raycasting/geometry/boundingVolumeHierarchy/AabbBoundingVolume.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
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

void ConstructiveSolidGeometry::getBounds(GeometryBounds& result) const
{
    if (operands.size() == 0) { result.setUnsupported(); return; }
    result.setEmpty();
    for (long int i = 0; i < operands.size(); ++i) {
        const CsgOperand* operand = operands[i];
        if (operand == nullptr || operand->getGeometry() == nullptr) {
            result.setIndeterminate(); return;
        }
        GeometryBounds child;
        operand->getGeometry()->getBounds(child);
        if (operand->getTransformation() != nullptr)
            child = child.transformed(operand->getTransformation()->transpose());
        result = result.enclosing(child);
    }
}

BoundingVolumeHierarchy *
ConstructiveSolidGeometry::createBoundingVolume() const
{
    AxisAlignedBoundingBox result = AxisAlignedBoundingBox::empty();
    for (long int i = 0; i < operands.size(); i++) {
        BoundingVolumeHierarchy *operandBounds = operands[i]->createBoundingVolume();
        result = result.enclosing(operandBounds->axisAlignedExtent());
        delete operandBounds;
    }
    return new AabbBoundingVolume(result);
}

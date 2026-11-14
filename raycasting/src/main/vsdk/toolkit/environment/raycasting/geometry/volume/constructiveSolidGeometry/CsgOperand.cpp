#include "vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "java/util/PriorityQueue.txx"
#include <typeinfo>

CsgOperand::CsgOperand(
    RayCastingGeometry *geometry,
    Material *material,
    Matrix4x4d *transformation,
    Matrix4x4d *transformationInverse,
    const java::ArrayList<TransformStep> &steps) :
    PovrayBody(geometry, material, nullptr, nullptr, false,
        java::ArrayList<PovrayBody*>(), java::ArrayList<PovrayBody*>(),
        transformation, transformationInverse, nullptr, nullptr, steps)
{
}

CsgOperand::CsgOperand(const CsgOperand &other) :
    PovrayBody(other)
{
}

bool
CsgOperand::denotesPlacedGeometry() const
{
    // Material and transform-step logs do not change the solid.
    return typeid(*this) == typeid(CsgOperand);
}

bool
CsgOperand::preservesCoreSolid() const
{
    return typeid(*this) == typeid(CsgOperand);
}

void
CsgOperand::translate(Vector3Dd *vector)
{
    applyTranslationToBodyTransform(vector);
    invalidateBakedBounds();
    if (geometryMaterial != nullptr) {
        geometryMaterial = geometryMaterial->translate(vector);
    }
}

void
CsgOperand::rotate(Vector3Dd *vector)
{
    applyRotationToBodyTransform(vector);
    invalidateBakedBounds();
    if (geometryMaterial != nullptr) {
        geometryMaterial = geometryMaterial->rotate(vector);
    }
}

void
CsgOperand::scale(Vector3Dd *vector)
{
    applyScaleToBodyTransform(vector);
    invalidateBakedBounds();
    if (geometryMaterial != nullptr) {
        geometryMaterial = geometryMaterial->scale(vector);
    }
}

void
CsgOperand::invert()
{
    if (geometry != nullptr) {
        markModified();
        getGeometry()->invertGeometry();
        bodySteps.add(TransformStep(TransformStep::Kind::Invert, Vector3Dd(0.0, 0.0, 0.0)));
    }
}

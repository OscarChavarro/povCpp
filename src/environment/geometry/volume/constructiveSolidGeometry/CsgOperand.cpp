#include "environment/geometry/volume/constructiveSolidGeometry/CsgOperand.h"
#include "environment/geometry/TransformedGeometry.h"
#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"

void
CsgOperand::translate(Vector3Dd *vector)
{
    if (TransformedGeometry *transformed =
            dynamic_cast<TransformedGeometry *>(geometry)) {
        transformed->translateGeometry(vector);
    } else if (ConstructiveSolidGeometry *csg =
            dynamic_cast<ConstructiveSolidGeometry *>(geometry)) {
        csg->translate(vector);
    } else {
        ensureMatrices();
        Matrix4x4d delta = Matrix4x4d().translation(
            vector->x(), vector->y(), vector->z()).transpose();
        Matrix4x4d deltaInverse = Matrix4x4d().translation(
            0.0 - vector->x(), 0.0 - vector->y(), 0.0 - vector->z()).transpose();
        *transformation = transformation->multiply(delta);
        *transformationInverse = deltaInverse.multiply(*transformationInverse);
    }
    if (material != nullptr) {
        material = material->translate(vector);
    }
}

void
CsgOperand::rotate(Vector3Dd *vector)
{
    if (TransformedGeometry *transformed =
            dynamic_cast<TransformedGeometry *>(geometry)) {
        transformed->rotateGeometry(vector);
    } else if (ConstructiveSolidGeometry *csg =
            dynamic_cast<ConstructiveSolidGeometry *>(geometry)) {
        csg->rotate(vector);
    } else {
        ensureMatrices();
        Matrix4x4d delta;
        Matrix4x4d deltaInverse;
        delta.axisRotationRodrigues(&deltaInverse, vector);
        *transformation = transformation->multiply(delta);
        *transformationInverse = deltaInverse.multiply(*transformationInverse);
    }
    if (material != nullptr) {
        material = material->rotate(vector);
    }
}

void
CsgOperand::scale(Vector3Dd *vector)
{
    if (TransformedGeometry *transformed =
            dynamic_cast<TransformedGeometry *>(geometry)) {
        transformed->scaleGeometry(vector);
    } else if (ConstructiveSolidGeometry *csg =
            dynamic_cast<ConstructiveSolidGeometry *>(geometry)) {
        csg->scale(vector);
    } else {
        ensureMatrices();
        Matrix4x4d delta = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
        Matrix4x4d deltaInverse = Matrix4x4d().scale(
            1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z());
        *transformation = transformation->multiply(delta);
        *transformationInverse = deltaInverse.multiply(*transformationInverse);
    }
    if (material != nullptr) {
        material = material->scale(vector);
    }
}

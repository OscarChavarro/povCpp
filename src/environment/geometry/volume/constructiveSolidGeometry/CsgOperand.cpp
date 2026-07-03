#include "environment/geometry/volume/constructiveSolidGeometry/CsgOperand.h"
#include "environment/geometry/volume/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"

void
CsgOperand::translate(Vector3Dd *vector)
{
    ensureMatrices();
    Matrix4x4d delta = Matrix4x4d().translation(
        vector->x(), vector->y(), vector->z()).transpose();
    Matrix4x4d deltaInverse = Matrix4x4d().translation(
        0.0 - vector->x(), 0.0 - vector->y(), 0.0 - vector->z()).transpose();
    *transformation = transformation->multiply(delta);
    *transformationInverse = deltaInverse.multiply(*transformationInverse);
    invalidateBakedBounds();
    steps.add(TransformStep(TransformStep::Kind::Translate, *vector));
    if (material != nullptr) {
        material = material->translate(vector);
    }
}

void
CsgOperand::rotate(Vector3Dd *vector)
{
    ensureMatrices();
    Matrix4x4d delta;
    Matrix4x4d deltaInverse;
    delta.axisRotationRodrigues(&deltaInverse, vector);
    *transformation = transformation->multiply(delta);
    *transformationInverse = deltaInverse.multiply(*transformationInverse);
    invalidateBakedBounds();
    steps.add(TransformStep(TransformStep::Kind::Rotate, *vector));
    if (material != nullptr) {
        material = material->rotate(vector);
    }
}

void
CsgOperand::scale(Vector3Dd *vector)
{
    ensureMatrices();
    Matrix4x4d delta = Matrix4x4d().scale(
        vector->x(), vector->y(), vector->z()).transpose();
    Matrix4x4d deltaInverse = Matrix4x4d().scale(
        1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z()).transpose();
    *transformation = transformation->multiply(delta);
    *transformationInverse = deltaInverse.multiply(*transformationInverse);
    invalidateBakedBounds();
    steps.add(TransformStep(TransformStep::Kind::Scale, *vector));
    if (material != nullptr) {
        material = material->scale(vector);
    }
}
